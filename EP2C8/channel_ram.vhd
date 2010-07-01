LIBRARY IEEE;
USE IEEE.STD_LOGIC_1164.all;
USE IEEE.NUMERIC_STD.ALL;

ENTITY channel_ram IS
	PORT (
		clk     : IN  STD_LOGIC;
		addr    : IN  STD_LOGIC_VECTOR(  8 DOWNTO 0 );
		data    : IN  STD_LOGIC_VECTOR( 17 DOWNTO 0 );
		we      : IN  STD_LOGIC;
		q       : OUT STD_LOGIC_VECTOR( 17 DOWNTO 0 )
	);
END ENTITY;

ARCHITECTURE a OF channel_ram IS

	TYPE ram_type IS ARRAY ( 0 TO 511 ) OF STD_LOGIC_VECTOR( 17 DOWNTO 0 );
	
	SIGNAL ram : ram_type := ( OTHERS => ( OTHERS => '0' ) );

BEGIN

	PROCESS( clk ) IS
	BEGIN
		IF FALLING_EDGE( clk ) THEN
			IF we = '1' THEN
				ram( TO_INTEGER( UNSIGNED( addr ) ) ) <= data;
			END IF;
			q <= ram( TO_INTEGER( UNSIGNED( addr ) ) );
		END IF;
	END PROCESS;

END ARCHITECTURE;
