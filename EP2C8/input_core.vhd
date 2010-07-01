LIBRARY IEEE;
USE IEEE.STD_LOGIC_1164.ALL;
USE IEEE.STD_LOGIC_UNSIGNED.ALL;
USE IEEE.NUMERIC_STD.ALL;

ENTITY input_core IS
	PORT (
			clk			: IN  STD_LOGIC;
			addr		: IN  STD_LOGIC_VECTOR( 1 DOWNTO 0 );
			q           : OUT STD_LOGIC_VECTOR( 7 DOWNTO 0 );
			cs			: IN  STD_LOGIC;
			quad_a		: IN  STD_LOGIC_VECTOR( 1 DOWNTO 0 );
			keys_a		: IN  STD_LOGIC_VECTOR( 3 DOWNTO 0 );
			quad_b		: IN  STD_LOGIC_VECTOR( 1 DOWNTO 0 );
			keys_b		: IN  STD_LOGIC_VECTOR( 3 DOWNTO 0 )
	);
END input_core;

ARCHITECTURE a OF input_core IS

	COMPONENT quadrature_decoder
		PORT (
			clk			: IN  STD_LOGIC;
			quad_a		: IN  STD_LOGIC;
			quad_b		: IN  STD_LOGIC;
			count		: OUT STD_LOGIC_VECTOR(  7 DOWNTO 0 );
			hold		: IN  STD_LOGIC := '0'
		);
	END COMPONENT;
	
	COMPONENT volume_control IS
		PORT (
			sample      : IN  STD_LOGIC_VECTOR( 7 DOWNTO 0 );
			volume      : IN  STD_LOGIC_VECTOR( 7 DOWNTO 0 );
			mute		: IN  STD_LOGIC;
			q           : OUT STD_LOGIC_VECTOR( 7 DOWNTO 0 )
		);
	END COMPONENT;
	
	SIGNAL count_a : STD_LOGIC_VECTOR( 7 DOWNTO 0 );
	SIGNAL count_b : STD_LOGIC_VECTOR( 7 DOWNTO 0 );
	SIGNAL lkeys_a : STD_LOGIC_VECTOR( 3 DOWNTO 0 );
	SIGNAL lkeys_b : STD_LOGIC_VECTOR( 3 DOWNTO 0 );
	
BEGIN

	qdec_a: quadrature_decoder PORT MAP (
		clk      => clk,
		quad_a   => quad_a( 0 ),
		quad_b   => quad_a( 1 ),
		count	 => count_a,
		hold     => cs
	);
	qdec_b: quadrature_decoder PORT MAP (
		clk      => clk,
		quad_a   => quad_b( 0 ),
		quad_b   => quad_b( 1 ),
		count	 => count_b,
		hold     => cs
	);
	
	-- latch keys when core is selected
	PROCESS( cs )
	BEGIN
		IF RISING_EDGE( cs ) THEN
			lkeys_a <= keys_a;
			lkeys_b <= keys_b;
		END IF;
	END PROCESS;
	
	-- map registers to address
	PROCESS( addr, count_a, lkeys_a, count_b, lkeys_b )
	BEGIN
		CASE addr IS
			WHEN "00" =>
				q <= count_a;
			WHEN "01" =>
				q <= "0000" & lkeys_a;
			WHEN "10" =>
				q <= count_b;
			WHEN "11" =>
				q <= "0000" & lkeys_b;
			WHEN OTHERS =>
		END CASE;
	END PROCESS;
	
END a;