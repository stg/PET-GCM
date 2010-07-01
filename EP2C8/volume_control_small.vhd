LIBRARY IEEE;
USE IEEE.STD_LOGIC_1164.ALL;
USE IEEE.STD_LOGIC_UNSIGNED.ALL;
USE IEEE.NUMERIC_STD.ALL;

-- Simple implementation (53 LC) of volume_control
-- Uses one's complement signed samples:
--   0x00-0x7F = +0...+127
--   0xFF-0x80 = -0...-127
-- Operation:
--   control/volume = 0...127/0...94.5% (100% is not possible)


ENTITY volume_control_small IS
	PORT (
			sample	: IN  STD_LOGIC_VECTOR( 7 DOWNTO 0 );
			control	: IN  STD_LOGIC_VECTOR( 6 DOWNTO 0 );
			q		: OUT STD_LOGIC_VECTOR( 7 DOWNTO 0 )
	);
END volume_control_small;

ARCHITECTURE a OF volume_control_small IS

	TYPE shift_type IS ARRAY ( 0 TO 6 ) OF STD_LOGIC_VECTOR( 7 DOWNTO 0 );
	SIGNAL sample_sh : shift_type;
	SIGNAL mute      : STD_LOGIC;
	SIGNAL word_i    : STD_LOGIC_VECTOR( 7 DOWNTO 0 );
	SIGNAL word_o    : STD_LOGIC_VECTOR( 7 DOWNTO 0 );

BEGIN

	word_i <= sample WHEN sample( 7 ) = '0' ELSE NOT sample;

	PROCESS( sample, control, word_i )
	BEGIN
		FOR i IN 0 TO 6 LOOP
			sample_sh( i )( 7 DOWNTO ( 7 - i ) ) <= ( OTHERS => '0' );
			IF control( 6 - i ) = '1' THEN
				sample_sh( i )( ( 6 - i ) DOWNTO 0 ) <= word_i( 7 DOWNTO ( i + 1 ) );
			ELSE
				sample_sh( i )( ( 6 - i ) DOWNTO 0 ) <= ( OTHERS => '0' );
			END IF;
		END LOOP;
	END PROCESS;
	
	word_o <= sample_sh( 0 ) + sample_sh( 1 ) + sample_sh( 2 ) + sample_sh( 3 )
	        + sample_sh( 4 ) + sample_sh( 5 ) + sample_sh( 6 );
	        
	q <= word_o WHEN sample( 7 ) = '0' ELSE NOT word_o;

END a;