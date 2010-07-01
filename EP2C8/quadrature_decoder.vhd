LIBRARY IEEE;
USE IEEE.STD_LOGIC_1164.ALL;
USE IEEE.STD_LOGIC_UNSIGNED.ALL;
USE IEEE.NUMERIC_STD.ALL;

ENTITY quadrature_decoder is
	PORT (
		-- clock
		clk                   : IN     STD_LOGIC;
		-- quadrature signals
		quad_a				  : IN     STD_LOGIC;
		quad_b				  : IN     STD_LOGIC;
		-- counter
		count				  : OUT    STD_LOGIC_VECTOR;
		-- hold
		hold				  : IN     STD_LOGIC := '0'
	);
END quadrature_decoder;

ARCHITECTURE a OF quadrature_decoder is

	SIGNAL shr_a      		  : STD_LOGIC_VECTOR( 2 DOWNTO 0 ) := ( others => '0' );
	SIGNAL shr_b      		  : STD_LOGIC_VECTOR( 2 DOWNTO 0 ) := ( others => '0' );
	SIGNAL count_enable       : STD_LOGIC;
	SIGNAL count_direction    : STD_LOGIC;
	SIGNAL counter            : STD_LOGIC_VECTOR( count'RANGE ) := ( others => '0' );

BEGIN

	count_enable <= shr_a( 1 ) XOR shr_a( 2 ) XOR shr_b( 1 ) XOR shr_b( 2 );
	count_direction <= shr_a( 1 ) XOR shr_b( 2 );

	PROCESS( hold )
	BEGIN
		IF RISING_EDGE( hold ) THEN
			count <= counter;
		END IF;
	END PROCESS;
	
	PROCESS( clk )
	BEGIN
		IF FALLING_EDGE( clk ) THEN
			shr_a <= shr_a( 1 DOWNTO 0 ) & quad_a;
			shr_b <= shr_b( 1 DOWNTO 0 ) & quad_b;
			IF count_enable = '1' THEN
				IF count_direction = '1' THEN
					counter <= counter + 1;
				ELSE
					counter <= counter - 1;
				END IF;
			END IF;
		END IF;
	END PROCESS;

END a;