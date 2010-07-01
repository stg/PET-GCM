LIBRARY IEEE;
USE IEEE.STD_LOGIC_1164.ALL;
USE IEEE.STD_LOGIC_UNSIGNED.ALL;
USE IEEE.NUMERIC_STD.ALL;

ENTITY audio_core IS
	PORT (
			clk			: IN  STD_LOGIC;
			addr		: IN  STD_LOGIC_VECTOR( 18 DOWNTO 0 );
			data		: IN  STD_LOGIC_VECTOR(  7 DOWNTO 0 );
			q           : OUT STD_LOGIC_VECTOR(  7 DOWNTO 0 );
			cs			: IN  STD_LOGIC;
			reg			: IN  STD_LOGIC;
			we          : IN  STD_LOGIC;
			ram_addr    : OUT STD_LOGIC_VECTOR( 18 DOWNTO 0 ) := ( others => '0' );
			ram_data    : IN  STD_LOGIC_VECTOR(  7 DOWNTO 0 );
			ram_q       : OUT STD_LOGIC_VECTOR(  7 DOWNTO 0 ) := ( others => '0' );
			ram_cs      : OUT STD_LOGIC := '1';
			ram_oe		: OUT STD_LOGIC := '1';
			ram_we      : OUT STD_LOGIC := '1';
			dma_int     : OUT STD_LOGIC := '0';
			audio_l		: OUT STD_LOGIC;
			audio_r		: OUT STD_LOGIC
	);
END audio_core;

ARCHITECTURE a OF audio_core IS

	COMPONENT volume_control
		PORT (
				sample      : IN  STD_LOGIC_VECTOR( 7 DOWNTO 0 );
				control     : IN  STD_LOGIC_VECTOR( 7 DOWNTO 0 );
				q           : OUT STD_LOGIC_VECTOR( 7 DOWNTO 0 )
		);
	END COMPONENT;

	TYPE	mode_type IS ARRAY( 0 TO 7 ) OF STD_LOGIC_VECTOR(  3 DOWNTO 0 );
	TYPE	addr_type IS ARRAY( 0 TO 7 ) OF STD_LOGIC_VECTOR( 18 DOWNTO 0 );
	TYPE    trig_type IS ARRAY( 0 TO 7 ) OF STD_LOGIC_VECTOR( 15 DOWNTO 0 );
	TYPE    data_type IS ARRAY( 0 TO 7 ) OF STD_LOGIC_VECTOR(  7 DOWNTO 0 );
	TYPE    spcm_type IS ARRAY( 0 TO 3 ) OF STD_LOGIC_VECTOR(  7 DOWNTO 0 );
	TYPE    rvol_type IS ARRAY( 0 TO 3 ) OF STD_LOGIC_VECTOR(  7 DOWNTO 0 );

	-- DMA channels
	SIGNAL  dma_clkdiv      : STD_LOGIC_VECTOR( 3 DOWNTO 0 );
	SIGNAL  dma_mode		: mode_type := ( OTHERS => ( OTHERS => '0' ) );
	SIGNAL  dma_addr_a		: addr_type := ( OTHERS => ( OTHERS => '0' ) );
	SIGNAL  dma_addr_b		: addr_type := ( OTHERS => ( OTHERS => '0' ) );
	SIGNAL  dma_addr		: addr_type := ( OTHERS => ( OTHERS => '0' ) );
	SIGNAL  dma_reload		: trig_type := ( OTHERS => ( OTHERS => '0' ) );
	SIGNAL  dma_count		: trig_type := ( OTHERS => ( OTHERS => '0' ) );
	SIGNAL  dma_prefetch	: data_type := ( OTHERS => ( OTHERS => '0' ) );
	SIGNAL  dma_data		: data_type := ( OTHERS => ( OTHERS => '0' ) );
	SIGNAL  dma_req   		: STD_LOGIC_VECTOR( 7 DOWNTO 0 ) := ( OTHERS => '0' );
	SIGNAL  dma_irq   		: STD_LOGIC_VECTOR( 7 DOWNTO 0 ) := ( OTHERS => '0' );
	SIGNAL  dma_loop        : STD_LOGIC_VECTOR( 7 DOWNTO 0 ) := ( OTHERS => '0' );
	
	-- CIA timer
	SIGNAL  cia_clkdiv      : STD_LOGIC_VECTOR( 15 DOWNTO 0 ) := ( OTHERS => '0' );
	SIGNAL  cia_clk         : STD_LOGIC := '0';
	SIGNAL  cia_int         : STD_LOGIC := '0';
	
	-- RAM access
	SIGNAL  ram_state 		: STD_LOGIC_VECTOR( 1 DOWNTO 0 ) := ( OTHERS => '0' );
	SIGNAL  ram_dma_ch 		: INTEGER RANGE 0 TO 7;
	
	-- Register access
	SIGNAL  reg_ch   		: INTEGER RANGE 0 TO 7;
	SIGNAL  vol_ch   		: INTEGER RANGE 4 TO 7;
	--SIGNAL  reg_volume      : rvol_type := ( OTHERS => ( OTHERS => '0' ) );
	SIGNAL  reg_temp        : STD_LOGIC_VECTOR( 15 DOWNTO 0 ) := ( OTHERS => '0' );
	
	-- Audio mixing
	SIGNAL  audio_out		: spcm_type := ( OTHERS => ( OTHERS => '0' ) );
	
	SIGNAL  audio_pwm		: STD_LOGIC_VECTOR( 8 DOWNTO 0 ) := ( OTHERS => '0' );
	SIGNAL  audio_mix_l		: STD_LOGIC_VECTOR( 8 DOWNTO 0 );
	SIGNAL  audio_mix_r		: STD_LOGIC_VECTOR( 8 DOWNTO 0 );
	
	-- CS triggering
	SIGNAL  cs_detect		: STD_LOGIC := '0';
	SIGNAL  cs_trig			: STD_LOGIC;

BEGIN

	vc0 : volume_control PORT MAP (
		sample	=> dma_data( 0 ),
		control	=> dma_data( 4 ),
		q		=> audio_out( 0 )
	);
	vc1 : volume_control PORT MAP (
		sample	=> dma_data( 1 ),
		control	=> dma_data( 5 ),
		q		=> audio_out( 1 )
	);
	vc2 : volume_control PORT MAP (
		sample	=> dma_data( 2 ),
		control	=> dma_data( 6 ),
		q		=> audio_out( 2 )
	);
	vc3 : volume_control PORT MAP (
		sample	=> dma_data( 3 ),
		control	=> dma_data( 7 ),
		q		=> audio_out( 3 )
	);

	dma_int <= '1' WHEN dma_irq /= "00000000" OR cia_int = '1' ELSE '0';
	reg_ch <= TO_INTEGER( UNSIGNED( addr( 6 DOWNTO 4 ) ) );
	vol_ch <= TO_INTEGER( UNSIGNED( addr( 5 DOWNTO 4 ) ) ) + 4;

	-- CS falls only shortly (less than 1/2 clk) between sequential writes
	-- This is used to detect the cs flank, triggering a new bus "event"
	PROCESS( cs, cs_detect )
	BEGIN
		IF cs_detect = '1' THEN
			cs_trig <= '1';
		ELSIF FALLING_EDGE( cs ) THEN
			cs_trig <= '0';
		END IF;
	END PROCESS;
	
	PROCESS( clk )
	BEGIN
	
		IF RISING_EDGE( clk ) THEN
		
			-- Mix outputs from volume controls
			audio_mix_l <= ( "0" & audio_out( 0 ) ) + ( "0" & audio_out( 3 ) );
			audio_mix_r <= ( "0" & audio_out( 1 ) ) + ( "0" & audio_out( 2 ) );
			
			-- Output mixed PCM audio as PWM @ 97656,25 hz
			IF audio_pwm( 8 DOWNTO 1 ) = "11111111" THEN
				audio_pwm <= ( others => '0' );
			ELSE
				audio_pwm <= audio_pwm + 1;
			END IF;
			IF audio_mix_l > audio_pwm THEN audio_l <= '1'; ELSE audio_l <= '0'; END IF;
			IF audio_mix_r > audio_pwm THEN audio_r <= '1'; ELSE audio_r <= '0'; END IF;
			
            -- Clock at 3571429 hz (close enough to 3579545 hz Amiga NTSC)
            IF dma_clkdiv = "1101" THEN
                -- Reset clock prescaler
                dma_clkdiv <= ( OTHERS => '0' );
                IF cia_clkdiv = "1000101110000001" THEN
                    cia_clkdiv <= ( OTHERS => '0' );
                    cia_clk <= NOT cia_clk;
                    IF cia_clk = '1' THEN
                        cia_int <= '1';
                    END IF;
                ELSE
                    cia_clkdiv <= cia_clkdiv + 1;
                END IF;
                -- Handle DMA channels
                FOR i IN 0 TO 7 LOOP
                    -- Time to apply prefetched sample?
                    IF dma_count( i ) = 0 THEN
                        -- Reset counter to reload value (determines sample period)
                        dma_count( i ) <= dma_reload( i );
                        -- Apply prefetched data
                        dma_data( i ) <= dma_prefetch( i );
                        -- DMA channel is active?
                        IF dma_mode( i )( 1 ) = '1' THEN
                            -- DMA is of mode FORWARD
                            IF dma_mode( i )( 0 ) = '0' THEN
                                -- Set next address (+1)
                                dma_addr( i ) <= dma_addr( i ) + 1;
                                -- Reached end of range?
                                IF dma_addr( i ) + 1 = dma_addr_b( i ) THEN
                                    IF dma_mode( i )( 3 ) = '0' THEN
                                        -- Disable channel (and trigger interrupt)
                                        dma_mode( i )( 1 DOWNTO 0 ) <= dma_mode( i )( 3 DOWNTO 2 );
                                    ELSE
                                        IF dma_mode( i )( 2 ) = '1' THEN
                                            -- Reverse direction
                                            dma_mode( i )( 0 ) <= '1';
                                        ELSE
                                            -- Request address reload
                                            dma_loop( i ) <= '1';
                                        END IF;
                                    END IF;
                                END IF;
                            -- DMA is of mode REVERSE
                            ELSE
                                -- Set next address (-1)
                                dma_addr( i ) <= dma_addr( i ) - 1;
                                -- Reached end of range?
                                IF dma_addr( i ) - 1 = dma_addr_a( i ) THEN
                                    IF dma_mode( i )( 3 ) = '0' THEN
                                        -- Disable channel (and trigger interrupt)
                                        dma_mode( i )( 1 DOWNTO 0 ) <= dma_mode( i )( 3 DOWNTO 2 );
                                    ELSE
                                        IF dma_mode( i )( 2 ) = '1' THEN
                                            -- Reverse direction
                                            dma_mode( i )( 0 ) <= '0';
                                        ELSE
                                            -- Request address reload
                                            dma_loop( i ) <= '1';
                                        END IF;
                                    END IF;
                                END IF;
                            END IF;
                            -- Request memory prefetch
                            dma_req( i ) <= '1';
                        END IF;
                    ELSE
                        -- Sample counter
                        dma_count( i ) <= dma_count( i ) - 1;
                    END IF;
                END LOOP;
            ELSE
                -- Count up clock prescaler
                dma_clkdiv <= dma_clkdiv + 1;
            END IF;
			
			IF cs = '1' AND cs_trig = '0' AND reg = '1' THEN
				cs_detect <= '1';
				IF we = '1' THEN

					-- Memory arrangement:
					--   Multibyte registers are arranged in little-endian
					--   All bytes in a multibyte register must be written atomically
					--   Multibyte registers must be written low-byte first

					-- Channel arrangement:
                    --   DMA#0 0x00-0x0F
                    --   DMA#1 0x10-0x1F
                    --   ...
                    --   DMA#7 0x70-0x7F
                    
                    -- Register arrangement:
                    --   DMA_PA   Addr. point A   0x01-0x03 R/W
                    --     Specifies 19-bit address point, A
                    --     5 MSB are "don't care"
                    --   DMA_PB   Addr. point B   0x05-0x07 R/W
                    --     Specifies 19-bit address point, B
                    --     5 MSB are "don't care"
                    --   DMA_IRQ  IRQ             0x08-0x08 R/-
                    --     IRQ bitmask for channels 0-7
                    --     If no bits are set, CIA interrupt is indicated
                    --     CIA interrupt is cleared upon reading
                    --   DMA_ADDR Current addr.   0x09-0x0B R/-
                    --     Currently playing address
                    --   DMA_RR   Sample interval 0x0C-0x0D R/W
                    --     Specifies 16-bit sample interval, I
                    --     Sample rate = 3571429 / ( I + 1 ) hz
                    --   DMA_VOL  Volume          0x0E-0x0E R/W
                    --     Specifies 8-bit volume
                    --     Range is 0-128, 129-255 treated as 128
                    --   DMA_MODE DMA mode        0x0F-0x0F R/W
                    --     Bits 0&1 specifies one of the following DMA modes:
                    --       00 DMA channel is disabled
                    --       10 DMA channel is enabled, incrementing
                    --       11 DMA channel is enabled, decrementing
                    --       01 Generate interrupt when next sample is read
                    --     Bits 2&3 specifies one of the following DMA automations:
                    --       00 Disable channel
                    --       01 Disable channel and generate interrupt
                    --       10 Reload starting address (loop)
                    --       11 Reverse direction (ping-pong)

                    -- Changes to sample interval or DMA mode take effect after the current
                    -- sample interval has finished processing.
                    
                    -- DMA automation is performed when the channel reaches the end of it's range:
                    --   addr. point B for incrementing operation, A for decrementing.
                    
                    -- When a DMA mode is set, the following occurs:
                    --   Any pending interrupt for the channel is cleared.
                    --   If the channel is currently disabled and modes 10/11 are set:
                    --     The starting point fot the channel is automatically loaded:
                    --       addr. point A for incrementing operation, B for decrementing.
                    --     If the interrupt is also cleared:
                    --       The channel fetches the next sample immediately.
                    --   If the channel is currently enabled and modes 10/11 are set:
                    --     The channel switches to the specified direction.

                    -- DMA channels 4-7 are volume controllers and when enabled, they are used to
                    -- modulate the volume of channels 0-3 respectively. When disabled, the volume
                    -- for each channel is fetched from the volume registers.
                    
                    -- DMA channels 0&1 are mixed to the LEFT audio channel, while
                    -- DMA channels 1&2 are mixed to the RIGHT audio channel.
                    
                    -- Common operations:
                    --
                    --   To reverse the direction of a currently active channel:
                    --
                    --     DMA_MODE <= DMA_MODE XOR 1
                    --
                    --     This takes effect AFTER the current sample period has ended.
                    --
                    --
                    --   To repeat the currently active channel from it's starting point:
                    --
                    --     DMA_MODE <= DMA_MODE
                    --
                    --     This reloads the starting address and begins operating at this address
                    --     AFTER the current sample period has ended.
                    --
                    --
                    --   To set up a channel for looping or ping-pong operation:
                    --
                    --     DMA_PA <= Sample address point A
                    --     DMA_PB <= Sample address point B
                    --     DMA_MODE <= 0b1010
                    --     DMA_PA <= Loop point sample address
                    --
                    --     The example above is for forward looping, the same process applies
                    --     to reverse and ping-pong modes.
                    --
                    --
                    --   To set a new address range for a sample:
                    --
                    --     DMA_PA <= New address point A
                    --     DMA_PB <= New address point B
                    --     DMA_MODE <= DMA_MODE
                    --
                    --     This should only be done on an interrupt for the specific channel,
                    --     ensuring a sample accurate transition to the new sample address as it
                    --     does not take effect until AFTER the current sample period has ended.
                    
					CASE addr( 3 DOWNTO 0 ) IS
						WHEN "0001" => -- Addr. point A 1'st byte (LSB)
							reg_temp( 7 DOWNTO 0 ) <= data;
						WHEN "0010" => -- Addr. point A 2'nd byte
							reg_temp( 15 DOWNTO 8 ) <= data;
						WHEN "0011" => -- Addr. point A 3'rd byte (MSB)
							dma_addr_a( reg_ch ) <= data( 2 DOWNTO 0 ) & reg_temp;
						WHEN "0101" => -- Addr. point B 1'st byte (LSB)
							reg_temp( 7 DOWNTO 0 ) <= data;
						WHEN "0110" => -- Addr. point B 2'nd byte
							reg_temp( 15 DOWNTO 8 ) <= data;
						WHEN "0111" => -- Addr. point B 3'rd byte (MSB)
							dma_addr_b( reg_ch ) <= data( 2 DOWNTO 0 ) & reg_temp;
						WHEN "1100" => -- Sample interval 1'st byte (LSB)
							reg_temp( 7 DOWNTO 0 ) <= data;
						WHEN "1101" => -- Sample interval 2'nd byte (MSB)
							dma_reload( reg_ch ) <= data & reg_temp( 7 DOWNTO 0 );
						WHEN "1110" => -- Volume
							IF dma_mode( vol_ch )( 1 ) = '0' THEN
                                dma_prefetch( vol_ch ) <= data;
                            END IF;
						WHEN "1111" => -- DMA mode
							dma_mode( reg_ch ) <= data( 3 DOWNTO 0 );
							-- Enabling a disabled channel?
							IF data( 1 ) = '1' AND dma_mode( reg_ch )( 1 ) = '0' THEN
								IF dma_mode( reg_ch )( 0 ) = '0' THEN
                                    -- Reset count, next sample is fetched immediately
                                    dma_count( reg_ch ) <= ( others => '0' );
								END IF;
								-- Load starting address
								IF data( 0 ) = '0' THEN
									dma_addr( reg_ch ) <= dma_addr_a( reg_ch );
								ELSE
									dma_addr( reg_ch ) <= dma_addr_b( reg_ch );
								END IF;
							END IF;
							IF data( 1 DOWNTO 0 ) = dma_mode( reg_ch )( 1 DOWNTO 0 ) THEN
								-- Load starting address
								IF data( 0 ) = '0' THEN
									dma_addr( reg_ch ) <= dma_addr_a( reg_ch );
								ELSE
									dma_addr( reg_ch ) <= dma_addr_b( reg_ch );
								END IF;
							END IF;
							-- Clear interrupt
							dma_irq( reg_ch ) <= '0';
						WHEN OTHERS =>
					END CASE;
				ELSE
					CASE addr( 3 DOWNTO 0 ) IS
						WHEN "0001" => -- Addr. point A 1'st byte (LSB)
							q <= dma_addr_a( reg_ch )( 7 DOWNTO 0 );
						WHEN "0010" => -- Addr. point A 2'nd byte
							q <= dma_addr_a( reg_ch )( 15 DOWNTO 8 );
						WHEN "0011" => -- Addr. point A 3'rd byte (MSB)
							q <= "00000" & dma_addr_a( reg_ch )( 18 DOWNTO 16 );
						WHEN "0101" => -- Addr. point B 1'st byte (LSB)
							q <= dma_addr_b( reg_ch )( 7 DOWNTO 0 );
						WHEN "0110" => -- Addr. point B 2'nd byte
							q <= dma_addr_b( reg_ch )( 15 DOWNTO 8 );
						WHEN "0111" => -- Addr. point B 3'rd byte (MSB)
							q <= "00000" & dma_addr_b( reg_ch )( 18 DOWNTO 16 );
						WHEN "1000" => -- DMA IRQ
                            IF cia_int = '1' THEN
                                q <= ( OTHERS => '0' );
                                cia_int <= '0';
                            ELSE
                                q <= dma_irq;
                            END IF;
						WHEN "1001" => -- Addr. 1'st byte (LSB)
							reg_temp <= "00000" & dma_addr( reg_ch )( 18 DOWNTO 8 );
							q <= dma_addr( reg_ch )( 7 DOWNTO 0 );
						WHEN "1010" => -- Addr. 2'nd byte
							q <= reg_temp( 7 DOWNTO 0 );
						WHEN "1011" => -- Addr. 3'rd byte (MSB)
							q <= reg_temp( 15 DOWNTO 8 );
						WHEN "1100" => -- Sample interval 1'st byte (MSB)
							q <= dma_reload( reg_ch )( 7 DOWNTO 0 );
						WHEN "1101" => -- Sample interval 2'nd byte (LSB)
							q <= dma_reload( reg_ch )( 15 DOWNTO 8 );
						WHEN "1110" => -- Volume
							q <= dma_prefetch( vol_ch );
						WHEN "1111" => -- DMA mode
							q <= "0000" & dma_mode( reg_ch );
						WHEN OTHERS =>
							q <= ( OTHERS => '0' );
					END CASE;
				END IF;
			ELSE
				cs_detect <= '0';
			END IF;
			
			CASE ram_state IS
				-- IDLE
				WHEN "00" =>
					IF cs = '1' AND cs_trig = '0' AND reg = '0' THEN
						cs_detect <= '1';
						IF we = '1' THEN
							-- Host WRITE
							ram_addr  <= addr;
							ram_q     <= data( 7 DOWNTO 0 );
							ram_cs    <= '0';
							ram_we    <= '0';
							ram_state <= "11";
						ELSE
							-- Host READ
							ram_addr  <= addr;
							ram_cs    <= '0';
							ram_oe    <= '0';
							ram_state <= "10";
						END IF;
					ELSE
						IF dma_req /= "00000000" THEN
							-- DMA channel READ
							IF dma_req( 0 ) = '1' THEN
								ram_addr <= dma_addr( 0 );
								ram_dma_ch <= 0;
							ELSIF dma_req( 1 ) = '1' THEN
								ram_addr <= dma_addr( 1 );
								ram_dma_ch <= 1;
							ELSIF dma_req( 2 ) = '1' THEN
								ram_addr <= dma_addr( 2 );
								ram_dma_ch <= 2;
							ELSIF dma_req( 3 ) = '1' THEN
								ram_addr <= dma_addr( 3 );
								ram_dma_ch <= 3;
							ELSIF dma_req( 4 ) = '1' THEN
								ram_addr <= dma_addr( 4 );
								ram_dma_ch <= 4;
							ELSIF dma_req( 5 ) = '1' THEN
								ram_addr <= dma_addr( 5 );
								ram_dma_ch <= 5;
							ELSIF dma_req( 6 ) = '1' THEN
								ram_addr <= dma_addr( 6 );
								ram_dma_ch <= 6;
							ELSIF dma_req( 7 ) = '1' THEN
								ram_addr <= dma_addr( 7 );
								ram_dma_ch <= 7;
							END IF;
							ram_cs <= '0';
							ram_oe <= '0';
							ram_state <= "01";
						END IF;
					END IF;
				-- DMA READ
				WHEN "01" =>
					dma_prefetch( ram_dma_ch ) <= ram_data;
					IF dma_mode( ram_dma_ch ) = "01" THEN
						dma_irq( ram_dma_ch ) <= '1';
					END IF;
					IF dma_loop( ram_dma_ch ) = '1' THEN
						IF dma_mode( ram_dma_ch )( 0 ) = '0' THEN
							dma_addr( ram_dma_ch ) <= dma_addr_a( ram_dma_ch ) - 1;
						ELSE
							dma_addr( ram_dma_ch ) <= dma_addr_b( ram_dma_ch ) + 1;
						END IF;
						dma_loop( ram_dma_ch ) <= '0';
					END IF;
					ram_cs <= '1';
					ram_oe <= '1';
					dma_req( ram_dma_ch ) <= '0';
					ram_state <= "00";
				-- Host READ
				WHEN "10" =>
					cs_detect <= '0';
					q <= ram_data;
					ram_cs <= '1';
					ram_oe <= '1';
					ram_state <= "00";
				-- Host WRITE
				WHEN "11" =>
					cs_detect <= '0';
					ram_cs <= '1';
					ram_we <= '1';
					ram_state <= "00";
				WHEN OTHERS =>
			END CASE;
		END IF;
	END PROCESS;

END a;