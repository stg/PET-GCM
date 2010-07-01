LIBRARY IEEE;
USE IEEE.STD_LOGIC_1164.ALL;
USE IEEE.STD_LOGIC_UNSIGNED.ALL;
USE IEEE.STD_LOGIC_ARITH.ALL;
USE IEEE.NUMERIC_STD.ALL;

ENTITY video_core IS
	PORT (
			clk		: IN     STD_LOGIC;
			addr	: IN     STD_LOGIC_VECTOR( 13 DOWNTO 0 );
			data	: IN     STD_LOGIC_VECTOR(  5 DOWNTO 0 );
			q		: OUT    STD_LOGIC_VECTOR(  5 DOWNTO 0 );
			cs		: IN     STD_LOGIC;
			we		: IN     STD_LOGIC;
			page	: IN     STD_LOGIC;
			dm_clk	: OUT    STD_LOGIC := '0';
			dm_le	: OUT    STD_LOGIC := '1';
			dm_oe	: OUT    STD_LOGIC := '0';
			dm_col	: BUFFER STD_LOGIC_VECTOR(  2 DOWNTO 0 ) := "000";
			dm_data	: OUT    STD_LOGIC_VECTOR(  7 DOWNTO 0 );
			rsync   : OUT    STD_LOGIC := '0'
	);
END video_core;

ARCHITECTURE a OF video_core IS

	-- Page data bus (8*18 signals)
	TYPE pdata_type   IS ARRAY ( 0 TO 7 ) OF STD_LOGIC_VECTOR( 17 DOWNTO 0 );
	-- Page address bus (2*
	TYPE paddr_type   IS ARRAY ( 0 TO 1 ) OF STD_LOGIC_VECTOR(  8 DOWNTO 0 );
	TYPE pactive_type IS ARRAY ( 0 TO 1 ) OF STD_LOGIC;

	COMPONENT channel_ram
		PORT(
			clk     : IN  STD_LOGIC;
			addr    : IN  STD_LOGIC_VECTOR(  8 DOWNTO 0 );
			data    : IN  STD_LOGIC_VECTOR( 17 DOWNTO 0 );
			we      : IN  STD_LOGIC;
			q       : OUT STD_LOGIC_VECTOR( 17 DOWNTO 0 )
		);
	END COMPONENT;
	
	-- Latched triad (rgb) management
	SIGNAL triad_data  : STD_LOGIC_VECTOR( 17 DOWNTO 0 );
	SIGNAL triad_latch : STD_LOGIC_VECTOR( 17 DOWNTO 0 ) := ( OTHERS => '0' );
	SIGNAL triad_we    : STD_LOGIC;

	-- Page currently being displayed
	SIGNAL page_disp    : pactive_type;
	
	-- Output from each page and channel
	SIGNAL page0_ch_out : pdata_type;
	SIGNAL page1_ch_out : pdata_type;
	
	-- Output from page associated with host (memory i/o)
	SIGNAL host_out     : pdata_type;
	
	-- Output from page associated with display (dotmatrix pwm)
	SIGNAL disp_out     : pdata_type;
	
	-- Output from channel select decoder
	SIGNAL ch_sel       : STD_LOGIC_VECTOR(  7 DOWNTO 0 );
	
	-- Channel address for each page
	SIGNAL page_addr    : paddr_type;
	
	-- Channel address for display (from PWM)
	SIGNAL disp_addr    : STD_LOGIC_VECTOR( 10 DOWNTO 0 ) := ( OTHERS => '0' );

	-- PWM clock divider
	SIGNAL pwm_clk      : STD_LOGIC_VECTOR(  1 DOWNTO 0 ) := ( OTHERS => '0' );
	
	-- Current PWM value
	SIGNAL pwm_v        : STD_LOGIC_VECTOR(  5 DOWNTO 0 ) := ( OTHERS => '0' );
	
	-- PWM generator state
	SIGNAL pwm_state    : STD_LOGIC_VECTOR(  2 DOWNTO 0 ) := ( OTHERS => '0' );
	
	-- PWM state delay counter
	SIGNAL pwm_delay    : STD_LOGIC_VECTOR(  5 DOWNTO 0 ) := ( OTHERS => '0' );
	
	-- Subsignals required for ModelSim simulation
	SIGNAL page0_addr   : STD_LOGIC_VECTOR(  8 DOWNTO 0 );
	SIGNAL page1_addr   : STD_LOGIC_VECTOR(  8 DOWNTO 0 );
	SIGNAL p0c0_we		: STD_LOGIC;
	SIGNAL p0c1_we		: STD_LOGIC;
	SIGNAL p0c2_we		: STD_LOGIC;
	SIGNAL p0c3_we		: STD_LOGIC;
	SIGNAL p0c4_we		: STD_LOGIC;
	SIGNAL p0c5_we		: STD_LOGIC;
	SIGNAL p0c6_we		: STD_LOGIC;
	SIGNAL p0c7_we		: STD_LOGIC;
	SIGNAL p1c0_we		: STD_LOGIC;
	SIGNAL p1c1_we		: STD_LOGIC;
	SIGNAL p1c2_we		: STD_LOGIC;
	SIGNAL p1c3_we		: STD_LOGIC;
	SIGNAL p1c4_we		: STD_LOGIC;
	SIGNAL p1c5_we		: STD_LOGIC;
	SIGNAL p1c6_we		: STD_LOGIC;
	SIGNAL p1c7_we		: STD_LOGIC;
	
BEGIN

	-- memory declaration
	--    2 pages
	--    8 channel/page
	--  384 word/channel
	--   18 bit/word
    p0c0: channel_ram PORT MAP (
	  clk	=> clk,
	  addr	=> page0_addr,
	  data	=> triad_data,
	  q		=> page0_ch_out( 0 ),
	  we	=> p0c0_we
	);
    p0c1: channel_ram PORT MAP (
	  clk	=> clk,
	  addr	=> page0_addr,
	  data	=> triad_data,
	  q		=> page0_ch_out( 1 ),
	  we	=> p0c1_we
	);
    p0c2: channel_ram PORT MAP (
	  clk	=> clk,
	  addr	=> page0_addr,
	  data	=> triad_data,
	  q		=> page0_ch_out( 2 ),
	  we	=> p0c2_we
	);
    p0c3: channel_ram PORT MAP (
	  clk	=> clk,
	  addr	=> page0_addr,
	  data	=> triad_data,
	  q		=> page0_ch_out( 3 ),
	  we	=> p0c3_we
	);
    p0c4: channel_ram PORT MAP (
	  clk	=> clk,
	  addr	=> page0_addr,
	  data	=> triad_data,
	  q		=> page0_ch_out( 4 ),
	  we	=> p0c4_we
	);
    p0c5: channel_ram PORT MAP (
	  clk	=> clk,
	  addr	=> page0_addr,
	  data	=> triad_data,
	  q		=> page0_ch_out( 5 ),
	  we	=> p0c5_we
	);
    p0c6: channel_ram PORT MAP (
	  clk	=> clk,
	  addr	=> page0_addr,
	  data	=> triad_data,
	  q		=> page0_ch_out( 6 ),
	  we	=> p0c6_we
	);
    p0c7: channel_ram PORT MAP (
	  clk	=> clk,
	  addr	=> page0_addr,
	  data	=> triad_data,
	  q		=> page0_ch_out( 7 ),
	  we	=> p0c7_we
	);
    p1c0: channel_ram PORT MAP (
	  clk	=> clk,
	  addr	=> page1_addr,
	  data	=> triad_data,
	  q		=> page1_ch_out( 0 ),
	  we	=> p1c0_we
	);
    p1c1: channel_ram PORT MAP (
	  clk	=> clk,
	  addr	=> page1_addr,
	  data	=> triad_data,
	  q		=> page1_ch_out( 1 ),
	  we	=> p1c1_we
	);
    p1c2: channel_ram PORT MAP (
	  clk	=> clk,
	  addr	=> page1_addr,
	  data	=> triad_data,
	  q		=> page1_ch_out( 2 ),
	  we	=> p1c2_we
	);
    p1c3: channel_ram PORT MAP (
	  clk	=> clk,
	  addr	=> page1_addr,
	  data	=> triad_data,
	  q		=> page1_ch_out( 3 ),
	  we	=> p1c3_we
	);
    p1c4: channel_ram PORT MAP (
	  clk	=> clk,
	  addr	=> page1_addr,
	  data	=> triad_data,
	  q		=> page1_ch_out( 4 ),
	  we	=> p1c4_we
	);
    p1c5: channel_ram PORT MAP (
	  clk	=> clk,
	  addr	=> page1_addr,
	  data	=> triad_data,
	  q		=> page1_ch_out( 5 ),
	  we	=> p1c5_we
	);
    p1c6: channel_ram PORT MAP (
	  clk	=> clk,
	  addr	=> page1_addr,
	  data	=> triad_data,
	  q		=> page1_ch_out( 6 ),
	  we	=> p1c6_we
	);
    p1c7: channel_ram PORT MAP (
	  clk	=> clk,
	  addr	=> page1_addr,
	  data	=> triad_data,
	  q		=> page1_ch_out( 7 ),
	  we	=> p1c7_we
	);

	-- Assign subsignals
	page0_addr <= page_addr( CONV_INTEGER( page_disp( 0 ) ) );
	page1_addr <= page_addr( CONV_INTEGER( page_disp( 1 ) ) );

	p0c0_we <= triad_we AND ch_sel( 0 ) AND page_disp( 1 );
	p0c1_we <= triad_we AND ch_sel( 1 ) AND page_disp( 1 );
	p0c2_we <= triad_we AND ch_sel( 2 ) AND page_disp( 1 );
	p0c3_we <= triad_we AND ch_sel( 3 ) AND page_disp( 1 );
	p0c4_we <= triad_we AND ch_sel( 4 ) AND page_disp( 1 );
	p0c5_we <= triad_we AND ch_sel( 5 ) AND page_disp( 1 );
	p0c6_we <= triad_we AND ch_sel( 6 ) AND page_disp( 1 );
	p0c7_we <= triad_we AND ch_sel( 7 ) AND page_disp( 1 );
	p1c0_we <= triad_we AND ch_sel( 0 ) AND page_disp( 0 );
	p1c1_we <= triad_we AND ch_sel( 1 ) AND page_disp( 0 );
	p1c2_we <= triad_we AND ch_sel( 2 ) AND page_disp( 0 );
	p1c3_we <= triad_we AND ch_sel( 3 ) AND page_disp( 0 );
	p1c4_we <= triad_we AND ch_sel( 4 ) AND page_disp( 0 );
	p1c5_we <= triad_we AND ch_sel( 5 ) AND page_disp( 0 );
	p1c6_we <= triad_we AND ch_sel( 6 ) AND page_disp( 0 );
	p1c7_we <= triad_we AND ch_sel( 7 ) AND page_disp( 0 );

	
	-- Triad latch (combines 3 8(6 used)-bit writes into one 18-bit write)
	PROCESS( clk, cs, we, addr( 1 DOWNTO 0 ) )
	BEGIN
		-- Latch first and second writes
		IF FALLING_EDGE( clk ) THEN
			IF cs = '1' AND addr( 1 ) = '0' AND we = '1' THEN
				IF addr( 0 ) = '0' THEN
					triad_data( 17 DOWNTO 12 ) <= data;
				ELSE
					triad_data( 11 DOWNTO  6 ) <= data;
				END IF;
			END IF;
		END IF;
	END PROCESS; 
	-- Direct pass-through for data and write-enable on third write
	triad_data( 5 DOWNTO  0 ) <= data;
	triad_we <= cs AND we AND addr( 1 );
	
	--triad_data <= data & triad_latch( 11 DOWNTO  6 ) & triad_latch(  5 DOWNTO  0 )
	--                WHEN addr( 1 DOWNTO 0 ) = "00" ELSE
	--              triad_latch( 17 DOWNTO 12 ) & data & triad_latch(  5 DOWNTO  0 )
	--                WHEN addr( 1 DOWNTO 0 ) = "01" ELSE
	--              triad_latch( 17 DOWNTO 12 ) & triad_latch( 11 DOWNTO  6 ) & data;
    --
	---- Triad latch (combines 3 8(6 used)-bit writes into one 18-bit write)
	--PROCESS( clk )
	--BEGIN
	--	IF FALLING_EDGE( clk ) THEN
	--		IF cs = '1' AND we = '1' THEN
	--			triad_we <= cs AND we AND addr( 1 );
	--			triad_latch <= triad_data;
	--		END IF;
	--	END IF;
	--END PROCESS;

	-- Map logical address to page, channel and word
	WITH addr( 7 DOWNTO 5 ) SELECT                        -- x / 8
		ch_sel <= "00000001" WHEN "000", "00000010" WHEN "001",
		          "00000100" WHEN "010", "00001000" WHEN "011",
		          "00010000" WHEN "100", "00100000" WHEN "101",
		          "01000000" WHEN "110", "10000000" WHEN "111",
		          ( OTHERS => 'X' ) WHEN OTHERS;
	page_addr( 0 )(  2 DOWNTO 0 ) <=      addr(  4 DOWNTO 2 ); -- x % 7
	page_addr( 0 )(  8 DOWNTO 3 ) <=      addr( 13 DOWNTO 8 ); -- y
	page_addr( 1 )                <= disp_addr(  8 downto 0 ); -- direct
	page_disp( 0 ) <= page;
	page_disp( 1 ) <= NOT page;

	-- Memory output page mux
	PROCESS( page, page0_ch_out, page1_ch_out )
	BEGIN
		IF page = '0' THEN
			host_out <= page0_ch_out;
			disp_out <= page1_ch_out;
		ELSE
			host_out <= page1_ch_out;
			disp_out <= page0_ch_out;
		END IF;
	END PROCESS;

	-- Memory output channel mux
	q <= host_out( CONV_INTEGER( addr( 7 DOWNTO 5 ) ) )( 17 downto 12 )
	       WHEN addr( 1 downto 0 ) = "00" ELSE
	     host_out( CONV_INTEGER( addr( 7 DOWNTO 5 ) ) )( 11 downto  6 )
	       WHEN addr( 1 downto 0 ) = "01" ELSE
	     host_out( CONV_INTEGER( addr( 7 DOWNTO 5 ) ) )(  5 downto  0 );

	-- Clock divider
	PROCESS( clk )
	BEGIN
		IF FALLING_EDGE( clk ) THEN
			pwm_clk <= pwm_clk + 1;
		END IF;
	END PROCESS;

	-- PWM comparator
	PROCESS( pwm_clk, disp_out, pwm_v, disp_addr( 10 DOWNTO 9 ) )
	BEGIN
		--IF FALLING_EDGE( pwm_clk( 1 ) ) THEN
			CASE disp_addr( 10 DOWNTO 9 ) IS
				WHEN "00" =>
					FOR i IN 0 TO 7 LOOP
						IF disp_out( i )( 17 DOWNTO 12 ) > pwm_v THEN dm_data( i ) <= '0'; ELSE dm_data( i ) <= '1'; END IF;
					END LOOP;			
				WHEN "01" =>
					FOR i IN 0 TO 7 LOOP
						IF disp_out( i )( 11 DOWNTO  6 ) > pwm_v THEN dm_data( i ) <= '0'; ELSE dm_data( i ) <= '1'; END IF;
					END LOOP;			
				WHEN others =>
					FOR i IN 0 TO 7 LOOP
						IF disp_out( i )(  5 DOWNTO  0 ) > pwm_v THEN dm_data( i ) <= '0'; ELSE dm_data( i ) <= '1'; END IF;
					END LOOP;			
			END CASE;
		--END IF;
	END PROCESS;

	-- PWM generator
	PROCESS( pwm_clk( 1 ), pwm_state, pwm_v, disp_addr )
	BEGIN
		-- pwm address:
		--  x(0...7) : disp_addr(  2 downto 0 )
		--  y(0..47) : disp_addr(  8 downto 3 )
		--  c(0...2) : disp_addr( 10 downto 9 )
		IF FALLING_EDGE( PWM_CLK( 1 ) ) THEN
			CASE pwm_state IS
				
				-- clock out
				WHEN "000" =>
					-- set clock out
					dm_clk <= '0';
					-- state = prepare next
					pwm_state <= "001";
				
				-- prepare next
				WHEN "001" =>
					-- reset clock out
					dm_clk <= '1';
					if disp_addr( 5 DOWNTO 3 ) = "111" then
						disp_addr( 5 DOWNTO 3 ) <= ( others => '0' );
						if disp_addr( 10 downto 9 ) = "10" then
							disp_addr( 10 DOWNTO 9 ) <= ( others => '0' );
							if disp_addr( 8 downto 6 ) = "101" then
								disp_addr( 8 downto 6 ) <= ( others => '0' );

								if disp_addr( 2 DOWNTO 0 ) = dm_col THEN
									-- yes: still on same column							
									-- state = latch pixels
									pwm_state <= "010";
								else
									-- no: still on same column
									-- state = switch column
									pwm_state <= "100";
								end if;
								-- reached last pwm cycle?
								if pwm_v = "111110" then
									-- yes: reached last pwm cycle
									-- reset pwm counter
									pwm_v <= "000000";
								else
									-- no: reached last pwm cycle
									-- increase pwm counter
									pwm_v <= pwm_v + 1;
								end if;
								
							else
								disp_addr( 8 downto 6 ) <= disp_addr( 8 downto 6 ) + 1;
								pwm_state <= "000";
							end if;
						else
							disp_addr( 10 downto 9 ) <= disp_addr( 10 downto 9 ) + 1;
							pwm_state <= "000";
						end if;
					else
						disp_addr( 5 downto 3 ) <= disp_addr( 5 downto 3 ) + 1;
						pwm_state <= "000";
					end if;
--					-- reached bottom row?
--					IF disp_addr( 8 DOWNTO 3 ) = "101111" then
--						-- reached bottom row
--						-- buffer page (prevents tearing)
--						--page_sel <= page;
--						-- reset row(y)
--						disp_addr( 8 DOWNTO 3 ) <= "000000";
--						-- reached last color?
--						IF disp_addr( 10 DOWNTO 9 ) = "10" then
--							-- yes: reached last color
--							-- reset color(c)
--							disp_addr( 10 DOWNTO 9 ) <= "00";
--							-- still on same column?							
--							IF disp_addr( 2 DOWNTO 0 ) = dm_col THEN
--								-- yes: still on same column							
--								-- state = latch pixels
--								pwm_state <= "010";
--							ELSE
--								-- no: still on same column
--								-- state = switch column
--								pwm_state <= "100";
--							END IF;
--							-- reached last pwm cycle?
--							IF pwm_v = "111110" THEN
--								-- yes: reached last pwm cycle
--								-- reset pwm counter
--								pwm_v <= "000000";
--							ELSE
--								-- no: reached last pwm cycle
--								-- increase pwm counter
--								pwm_v <= pwm_v + 1;
--							END IF;
--						ELSE
--							-- no: reached last color
--							-- set next color(c)
--							disp_addr( 10 DOWNTO 9 ) <= disp_addr( 10 DOWNTO 9 ) + 1;
--							-- state = clock out
--							pwm_state <= "000";
--						END IF;
--					ELSE
--						-- no: reached bottom row
--						-- set next row(y)
--						disp_addr( 8 DOWNTO 3 ) <= disp_addr( 8 DOWNTO 3 ) + 1;
--						-- state = clock out
--						pwm_state <= "000";
--					END IF;

				-- latch pixels
				WHEN "010" =>
					-- set latch
					dm_le <= '0';
					-- state: reset latch
					pwm_state <= "011";
					-- reached last pwm cycle?
					IF pwm_v = "000000" THEN
						-- yes: reached last pwm cycle
						-- set next column(x)
						disp_addr( 2 DOWNTO 0 ) <= disp_addr( 2 DOWNTO 0 ) + 1;
					END IF;
					
				-- reset latch
				WHEN "011" =>
					-- reset latch
					dm_le <= '1';
					-- enable output
					dm_oe <= '1';
					-- state = clock out
					pwm_state <= "000";
				
				-- switch column
				WHEN "100" =>
					-- disable output
					dm_oe <= '0';
					-- allow time for host to page-switch (wrong place?)
					IF pwm_delay = "100000" THEN
						-- reset delay
						pwm_delay <= ( others => '0' );
						-- state = set column selector
						pwm_state <= "101";
						-- deassert rsync
						rsync <= '0';
					ELSE
						-- delay counter
						pwm_delay <= pwm_delay + 1;
						-- assert rsync for last column
						IF dm_col = "111" THEN rsync <= '1'; ELSE rsync <= '0'; END IF;
					END IF;
				
				-- set column selector	
				WHEN OTHERS =>
					-- select new column
					dm_col <= disp_addr( 2 DOWNTO 0 );
					-- state = latch pixels
					pwm_state <= "010";
				
			END CASE;
		END IF;
	END PROCESS;
	
END a;