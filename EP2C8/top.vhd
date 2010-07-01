LIBRARY IEEE;
USE IEEE.STD_LOGIC_1164.ALL;
USE IEEE.STD_LOGIC_UNSIGNED.ALL;
USE IEEE.NUMERIC_STD.ALL;

ENTITY top IS
	PORT (
		-- clock
		clk						: IN     STD_LOGIC;
		clk_in					: IN     STD_LOGIC;
		-- display matrix
		dm_clk, dm_le, dm_oe	: OUT    STD_LOGIC := '1';
		dm_col					: BUFFER STD_LOGIC_VECTOR(  2 DOWNTO 0 ) := "000";
		dm_data					: OUT    STD_LOGIC_VECTOR(  7 DOWNTO 0 );
		-- eZ80-compatible bus
		rd, wr, cs				: IN     STD_LOGIC;
		addr					: IN     STD_LOGIC_VECTOR( 19 DOWNTO 0 );
		data					: INOUT  STD_LOGIC_VECTOR(  7 DOWNTO 0 );
		reset                   : OUT    STD_LOGIC;
		-- control signals
		page					: IN     STD_LOGIC;
		-- input devices
		quad_a					: IN     STD_LOGIC_VECTOR(  1 DOWNTO 0 );
		keys_a					: IN     STD_LOGIC_VECTOR(  3 DOWNTO 0 );
		quad_b					: IN     STD_LOGIC_VECTOR(  1 DOWNTO 0 );
		keys_b					: IN     STD_LOGIC_VECTOR(  3 DOWNTO 0 );
		-- interrupts
		video_int           	: OUT    STD_LOGIC;
		audio_int       		: OUT    STD_LOGIC;
		-- sram
		ram_addr				: OUT    STD_LOGIC_VECTOR( 18 DOWNTO 0 );
		ram_data				: INOUT  STD_LOGIC_VECTOR(  7 DOWNTO 0 );
		ram_cs					: OUT    STD_LOGIC;
		ram_oe					: OUT    STD_LOGIC;
		ram_we					: OUT    STD_LOGIC;
		-- audio out
		audio_l					: OUT    STD_LOGIC;
		audio_r					: OUT    STD_LOGIC;
		-- peripherals
		led						: OUT    STD_LOGIC;
		reset_n					: IN     STD_LOGIC

	);
END top;

ARCHITECTURE a OF top IS

	COMPONENT video_core
		PORT (
			clk			: IN  STD_LOGIC;
			addr		: IN  STD_LOGIC_VECTOR( 13 DOWNTO 0 );
			data		: IN  STD_LOGIC_VECTOR(  5 DOWNTO 0 );
			q           : OUT STD_LOGIC_VECTOR(  5 DOWNTO 0 );
			cs			: IN  STD_LOGIC;
			we          : IN  STD_LOGIC;
			page        : IN  STD_LOGIC;
			dm_clk		: OUT STD_LOGIC;
			dm_le 		: OUT STD_LOGIC;
			dm_oe 		: OUT STD_LOGIC;
			dm_col		: OUT STD_LOGIC_VECTOR(  2 DOWNTO 0 );
			dm_data		: OUT STD_LOGIC_VECTOR(  7 DOWNTO 0 );
			rsync       : OUT STD_LOGIC
		);
	END COMPONENT;

	COMPONENT audio_core
		PORT (
			clk			: IN  STD_LOGIC;
			addr		: IN  STD_LOGIC_VECTOR( 18 DOWNTO 0 );
			data		: IN  STD_LOGIC_VECTOR(  7 DOWNTO 0 );
			q           : OUT STD_LOGIC_VECTOR(  7 DOWNTO 0 );
			cs			: IN  STD_LOGIC;
			reg			: IN  STD_LOGIC;
			we          : IN  STD_LOGIC;
			ram_addr    : OUT STD_LOGIC_VECTOR( 18 DOWNTO 0 );
			ram_data    : IN  STD_LOGIC_VECTOR(  7 DOWNTO 0 );
			ram_q       : OUT STD_LOGIC_VECTOR(  7 DOWNTO 0 );
			ram_cs      : OUT STD_LOGIC;
			ram_oe		: OUT STD_LOGIC;
			ram_we      : OUT STD_LOGIC;
			dma_int		: OUT STD_LOGIC;
			audio_l		: OUT STD_LOGIC;
			audio_r		: OUT STD_LOGIC
		);
	END COMPONENT;

	COMPONENT input_core
		PORT (
			clk			: IN  STD_LOGIC;
			addr		: IN  STD_LOGIC_VECTOR(  1 DOWNTO 0 );
			q           : OUT STD_LOGIC_VECTOR(  7 DOWNTO 0 );
			cs			: IN  STD_LOGIC;
			quad_a		: IN  STD_LOGIC_VECTOR(  1 DOWNTO 0 );
			keys_a		: IN  STD_LOGIC_VECTOR(  3 DOWNTO 0 );
			quad_b		: IN  STD_LOGIC_VECTOR(  1 DOWNTO 0 );
			keys_b		: IN  STD_LOGIC_VECTOR(  3 DOWNTO 0 )
		);
	END COMPONENT;
	
	SIGNAL video_cs		: STD_LOGIC;
	SIGNAL input_cs		: STD_LOGIC;
	SIGNAL audio_cs		: STD_LOGIC;
	SIGNAL audio_cs_reg	: STD_LOGIC;
	SIGNAL audio_cs_mem	: STD_LOGIC;
	SIGNAL video_q		: STD_LOGIC_VECTOR(  5 DOWNTO 0 ); 
	SIGNAL input_q		: STD_LOGIC_VECTOR(  7 DOWNTO 0 ); 
	SIGNAL audio_q		: STD_LOGIC_VECTOR(  7 DOWNTO 0 ); 
	
	SIGNAL ram_q		: STD_LOGIC_VECTOR(  7 DOWNTO 0 );
	SIGNAL ram_dir		: STD_LOGIC;
	
	SIGNAL temp1		: STD_LOGIC_VECTOR( 15 DOWNTO 0 ) := ( OTHERS => '0' );
	SIGNAL temp2        : STD_LOGIC_VECTOR(  7 DOWNTO 0 ) := ( OTHERS => '0' );
	
	SIGNAL wr_i, cs_i   : STD_LOGIC := '0';
	SIGNAL bus_state    : STD_LOGIC_VECTOR( 1 DOWNTO 0 ) := "00";

BEGIN

    reset <= 'Z' WHEN reset_n = '1' ELSE '0';

	--LED fader
	PROCESS( clk )
	BEGIN
		IF FALLING_EDGE( clk ) THEN
			IF temp1 = "1111111111111110" THEN
				temp1 <= ( others => '0' );
				temp2 <= temp2 + 1;
			ELSE
				temp1 <= temp1 + 1;
			END IF;
			IF temp2 > temp1( 15 DOWNTO 8 ) THEN led <= '1'; ELSE led <= '0'; END IF;
		END IF;
	END PROCESS;

	vcore : video_core PORT MAP (
		clk  		=> clk,
		addr		=> addr( 13 DOWNTO 0 ),
		data		=> data(  7 DOWNTO 2 ),
		q			=> video_q,
		cs			=> video_cs,
		we			=> wr_i,
		page		=> page,
		dm_clk		=> dm_clk,
		dm_le 		=> dm_le,
		dm_oe 		=> dm_oe,
		dm_col		=> dm_col,
		dm_data		=> dm_data,
		rsync		=> video_int
	);

	acore : audio_core PORT MAP (
		clk  		=> clk,
		addr		=> addr( 18 DOWNTO 0 ),
		data		=> data(  7 DOWNTO 0 ),
		q			=> audio_q,
		cs			=> audio_cs,
		reg			=> audio_cs_reg,
		we			=> wr_i,

		ram_addr    => ram_addr,
		ram_data    => ram_data,
		ram_q       => ram_q,
		ram_cs      => ram_cs,
		ram_oe		=> ram_dir,
		ram_we      => ram_we,

		dma_int     => audio_int,

		audio_l		=> audio_l,
		audio_r		=> audio_r
	);

	icore : input_core PORT MAP (
		clk  		=> clk,
		addr		=> addr(  1 DOWNTO 0 ),
		q			=> input_q,
		cs			=> input_cs,
		quad_a		=> quad_a,
		keys_a		=> keys_a,
		quad_b		=> quad_b,
		keys_b		=> keys_b
	);
	
	-- Memory arrangement:
	--   Video           0x00000-0x3FFFF
	--   Input           0x40000-0x5FFFF
	--   Audio registers 0x60000-0x7FFFF
	--   Audio memory    0x80000-0xFFFFF

	video_cs     <= cs_i AND ( NOT addr( 19 ) ) AND ( NOT addr( 18 ) );
	input_cs     <= cs_i AND ( NOT addr( 19 ) ) AND       addr( 18 ) AND ( NOT addr( 17 ) );
	audio_cs_reg <= cs_i AND ( NOT addr( 19 ) ) AND       addr( 18 ) AND       addr( 17 );
	audio_cs_mem <= cs_i AND       addr( 19 );
	audio_cs     <= audio_cs_reg OR audio_cs_mem;
	
	cs_i <= ( NOT cs ) AND ( ( NOT wr ) OR ( NOT rd ) );
	wr_i <= NOT wr;
	
	--PROCESS( clk )
	--BEGIN
	--	IF FALLING_EDGE( clk ) THEN
	--		CASE bus_state IS
	--			WHEN "00" =>  -- Wait for R/W
	--				IF cs = '0' AND ( rd = '0' OR wr = '0' ) THEN
	---					IF wr = '0' THEN
	--						wr_i <= '1';
	--					END IF;
	--					bus_state <= "01";
	--				END IF;
	--			WHEN "01" => -- Assert CS
	--				cs_i <= '1';
	--				bus_state <= "10";
	--			WHEN "10" => -- Allow processing
	--				bus_state <= "11";
	--			WHEN "11" => -- Wait for RD/WR to deassert
	--				IF wr = '1' AND rd = '1' THEN
	--					wr_i <= '0';
	--					bus_state <= "00";
	--				END IF;
	--				cs_i <= '0';
	--		END CASE;
	--	END IF;
	--END PROCESS;
	
	PROCESS( rd, video_cs, input_cs, audio_cs, video_q, audio_q, input_q )
	BEGIN
		IF video_cs = '1' AND rd = '0' THEN
			data <= video_q & "00";
		ELSIF input_cs = '1' AND rd = '0' THEN
			data <= input_q;
		ELSIF audio_cs = '1' AND rd = '0' THEN
			data <= audio_q;
		ELSE
			data <= ( others => 'Z' );
		END IF;
	END PROCESS;
	
	PROCESS( ram_dir, ram_q )
	BEGIN
		ram_oe <= ram_dir;
		IF ram_dir = '0' THEN
			ram_data <= ( others => 'Z' );
		ELSE
			ram_data <= ram_q;
		END IF;
	END PROCESS;
	
END a;