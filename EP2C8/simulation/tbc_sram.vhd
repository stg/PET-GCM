LIBRARY IEEE;
USE IEEE.STD_LOGIC_1164.all;
USE IEEE.NUMERIC_STD.ALL;

ENTITY sram_4mb_10ns IS
  PORT (
    addr : IN    STD_LOGIC_VECTOR( 18 DOWNTO 0 );
    data : INOUT STD_LOGIC_VECTOR(  7 DOWNTO 0 );
    cs   : IN    STD_LOGIC;
    oe   : IN    STD_LOGIC;
    we   : IN    STD_LOGIC
  );
END ENTITY;

ARCHITECTURE a OF sram_4mb_10ns IS

  TYPE ram_type IS ARRAY ( 0 TO 524287 ) OF STD_LOGIC_VECTOR( 7 DOWNTO 0 );
  
  SIGNAL ram : ram_type := ( OTHERS => ( OTHERS => '0' ) );

BEGIN

  PROCESS( cs, oe, we, data, addr )
  BEGIN
    IF cs = '0' AND oe = '0' THEN
		data <= ram( TO_INTEGER( UNSIGNED( addr ) ) );
    ELSE
		data <= ( OTHERS => 'Z' );
    END IF;
    IF cs = '0' AND RISING_EDGE( we ) THEN
		ram( TO_INTEGER( UNSIGNED( addr ) ) ) <= data;
	END IF;
  END PROCESS;

END ARCHITECTURE;
