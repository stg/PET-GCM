LIBRARY ieee;
USE ieee.std_logic_1164.all;

LIBRARY lpm;
USE lpm.all;

ENTITY pwm_vol IS
	PORT (
		sample : in  STD_LOGIC_VECTOR(7 DOWNTO 0);
		volume : IN  STD_LOGIC_VECTOR(7 DOWNTO 0);
		q      : OUT STD_LOGIC_VECTOR(7 DOWNTO 0)
	);
END pwm_vol;

ARCHITECTURE a OF pwm_vol IS

	--SIGNAL sub_q : STD_LOGIC_VECTOR(7 DOWNTO 0);

	COMPONENT lpm_mult
	GENERIC (
		lpm_hint		: STRING;
		lpm_representation		: STRING;
		lpm_type		: STRING;
		lpm_widtha		: NATURAL;
		lpm_widthb		: NATURAL;
		lpm_widthp		: NATURAL
	);
	PORT (
			dataa	: IN STD_LOGIC_VECTOR (7 DOWNTO 0);
			datab	: IN STD_LOGIC_VECTOR (7 DOWNTO 0);
			result	: OUT STD_LOGIC_VECTOR (15 DOWNTO 0)
	);
	END COMPONENT;

BEGIN
	--q    <= sub_q;

	multiplier : lpm_mult
	GENERIC MAP (
		lpm_hint           => "MAXIMIZE_SPEED=5",
		lpm_representation => "UNSIGNED",
		lpm_type           => "LPM_MULT",
		lpm_widtha         => 8,
		lpm_widthb         => 8,
		lpm_widthp         => 16
	)
	PORT MAP (
		dataa                 => sample,
		datab                 => volume,
		result( 14 DOWNTO 7 ) => q -- sub_q
	);

END a;