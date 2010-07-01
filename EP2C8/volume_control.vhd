LIBRARY IEEE;
USE IEEE.STD_LOGIC_1164.ALL;
USE IEEE.STD_LOGIC_UNSIGNED.ALL;
USE IEEE.NUMERIC_STD.ALL;

-- Full implementation (93 LC) of volume_control
-- Uses two's complement signed samples:
--   0x00-0x7F = +0...+127
--   0xFF-0x80 = -1...-128
-- Operation when allow_gain = TRUE   (+15 LC)
--   control/volume = 0...255/0...199% (may wrap and distort)
-- Operation when allow_gain = FALSE  (+ 0 LC)
--   control/volume = 0...128/0...100%, 129...255/100%

ENTITY volume_control IS
	GENERIC (
			allow_gain : BOOLEAN := FALSE -- Requires additional gates
	);
	PORT (
			sample      : IN  STD_LOGIC_VECTOR( 7 DOWNTO 0 );
			control     : IN  STD_LOGIC_VECTOR( 7 DOWNTO 0 );
			q           : OUT STD_LOGIC_VECTOR( 7 DOWNTO 0 )
	);
END volume_control;

ARCHITECTURE a OF volume_control IS

	TYPE shift_type IS ARRAY ( 0 TO 7 ) OF STD_LOGIC_VECTOR( 7 DOWNTO 0 );
	SIGNAL sample_sh : shift_type;
	--SIGNAL volume    : STD_LOGIC_VECTOR( 7 DOWNTO 0 );
	--SIGNAL mute      : STD_LOGIC;
	SIGNAL word_i    : STD_LOGIC_VECTOR( 7 DOWNTO 0 );
	SIGNAL word_o    : STD_LOGIC_VECTOR( 7 DOWNTO 0 );
	SIGNAL q_signed  : STD_LOGIC_VECTOR( 7 DOWNTO 0 );

BEGIN

	word_i <= sample WHEN sample( 7 ) = '0' ELSE ( NOT sample ) + 1;

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
		IF allow_gain = TRUE AND control( 7 ) = '1' THEN
			sample_sh( 7 ) <= word_i;
		ELSE
			sample_sh( 7 ) <= ( others => '0' );
		END IF;
	END PROCESS;
	
	word_o <= sample_sh( 0 ) + sample_sh( 1 ) + sample_sh( 2 ) + sample_sh( 3 )
	        + sample_sh( 4 ) + sample_sh( 5 ) + sample_sh( 6 ) + sample_sh( 7 );
	        
	q_signed <= sample WHEN allow_gain = FALSE AND control( 7 ) = '1' ELSE
				word_o WHEN sample( 7 ) = '0' ELSE
				NOT ( word_o - 1 );
				
	q <= q_signed XOR "10000000";
	
	PROCESS( control, word_o, sample( 7 ), q_signed )
	BEGIN
		--IF allow_gain = FALSE AND control( 7 ) = '1' THEN
		--	q_signed <= sample;
		--ELSIF sample( 7 ) = '0' THEN
		--	q_signed <= word_o;
		--ELSE
		--	q_signed <= NOT ( word_o - 1 );
		--END IF;
		----q <= q_signed + 128;
		--IF q_signed( 7 ) == "1" THEN
		--	q <= "0" & NOT q_signed( 6 DOWNTO 0 );
		--ELSE
		--	q <= "1" & q_signed( 6 DOWNTO 0 );
		--END IF;
	END PROCESS;

END a;