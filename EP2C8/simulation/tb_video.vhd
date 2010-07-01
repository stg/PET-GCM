LIBRARY IEEE;
USE IEEE.STD_LOGIC_1164.ALL;

ENTITY tb_video IS END;

ARCHITECTURE a OF tb_video IS

  COMPONENT top IS
    PORT (
      clk                  : IN     STD_LOGIC;
      dm_clk, dm_le, dm_oe : OUT    STD_LOGIC := '1';
      dm_col               : BUFFER STD_LOGIC_VECTOR(  2 DOWNTO 0 ) := "000";
      dm_data              : OUT    STD_LOGIC_VECTOR(  7 DOWNTO 0 );
      rd, wr, cs           : IN     STD_LOGIC;
      addr                 : IN     STD_LOGIC_VECTOR( 19 DOWNTO 0 );
      data                 : INOUT  STD_LOGIC_VECTOR(  7 DOWNTO 0 );
      delay                : OUT    STD_LOGIC;
      page                 : IN     STD_LOGIC;
      quad_a               : IN     STD_LOGIC_VECTOR(  1 DOWNTO 0 );
      keys_a               : IN     STD_LOGIC_VECTOR(  3 DOWNTO 0 );
      quad_b               : IN     STD_LOGIC_VECTOR(  1 DOWNTO 0 );
      keys_b               : IN     STD_LOGIC_VECTOR(  3 DOWNTO 0 );
      rsync                : OUT    STD_LOGIC;
      reload               : OUT    STD_LOGIC;
      ram_addr             : OUT    STD_LOGIC_VECTOR( 18 DOWNTO 0 );
      ram_data             : INOUT  STD_LOGIC_VECTOR(  7 DOWNTO 0 );
      ram_cs               : OUT    STD_LOGIC;
      ram_oe               : OUT    STD_LOGIC;
      ram_we               : OUT    STD_LOGIC;
      audio_l              : OUT    STD_LOGIC;
      audio_r              : OUT    STD_LOGIC;
      led                  : OUT    STD_LOGIC;
      reset                : IN     STD_LOGIC
    );
  END COMPONENT;
  
  COMPONENT sram_4mb_10ns IS
    PORT (
      addr : IN    STD_LOGIC_VECTOR( 18 DOWNTO 0 );
      data : INOUT STD_LOGIC_VECTOR(  7 DOWNTO 0 );
      cs   : IN    STD_LOGIC;
      oe   : IN    STD_LOGIC;
      we   : IN    STD_LOGIC
    );
  END COMPONENT;

  SIGNAL clk      : STD_LOGIC := '0';
  SIGNAL dm_clk   : STD_LOGIC;
  SIGNAL dm_le    : STD_LOGIC;
  SIGNAL dm_oe    : STD_LOGIC;
  SIGNAL dm_col   : STD_LOGIC_VECTOR(  2 DOWNTO 0 );
  SIGNAL dm_data  : STD_LOGIC_VECTOR(  7 DOWNTO 0 );
  SIGNAL rd       : STD_LOGIC := '1';
  SIGNAL wr       : STD_LOGIC := '1';
  SIGNAL cs       : STD_LOGIC := '1';
  SIGNAL addr     : STD_LOGIC_VECTOR( 19 DOWNTO 0 ) := ( OTHERS => '0' );
  SIGNAL data     : STD_LOGIC_VECTOR(  7 DOWNTO 0 ) := ( OTHERS => '0' );
  SIGNAL delay    : STD_LOGIC;
  SIGNAL page     : STD_LOGIC := '0';
  SIGNAL quad_a   : STD_LOGIC_VECTOR(  1 DOWNTO 0 ) := ( OTHERS => '0' );
  SIGNAL keys_a   : STD_LOGIC_VECTOR(  3 DOWNTO 0 ) := ( OTHERS => '0' );
  SIGNAL quad_b   : STD_LOGIC_VECTOR(  1 DOWNTO 0 ) := ( OTHERS => '0' );
  SIGNAL keys_b   : STD_LOGIC_VECTOR(  3 DOWNTO 0 ) := ( OTHERS => '0' );
  SIGNAL rsync    : STD_LOGIC;
  SIGNAL reload   : STD_LOGIC;
  SIGNAL ram_addr : STD_LOGIC_VECTOR( 18 DOWNTO 0 ) := ( OTHERS => '0' );
  SIGNAL ram_data : STD_LOGIC_VECTOR(  7 DOWNTO 0 ) := ( OTHERS => '0' );
  SIGNAL ram_cs   : STD_LOGIC;
  SIGNAL ram_oe   : STD_LOGIC;
  SIGNAL ram_we   : STD_LOGIC;
  SIGNAL audio_l  : STD_LOGIC;
  SIGNAL audio_r  : STD_LOGIC;
  SIGNAL led      : STD_LOGIC;
  SIGNAL reset    : STD_LOGIC := '0';

BEGIN
  top_tb : top PORT MAP (
    clk      => clk,
    dm_clk   => dm_clk,
    dm_le    => dm_le,
    dm_oe    => dm_oe,
    dm_col   => dm_col,
    dm_data  => dm_data,
    rd       => rd,
    wr       => wr,
    cs       => cs,
    addr     => addr,
    data     => data,
    delay    => delay,
    page     => page,
    quad_a   => quad_a,
    keys_a   => keys_a,
    quad_b   => quad_b,
    keys_b   => keys_b,
    rsync    => rsync,
    reload   => reload,
    ram_addr => ram_addr,
    ram_data => ram_data,
    ram_cs   => ram_cs,
    ram_oe   => ram_oe,
    ram_we   => ram_we,
    audio_l  => audio_l,
    audio_r  => audio_r,
    led      => led,
    reset    => reset
  );
  
  sram_tb : sram_4mb_10ns PORT MAP (
    addr     => ram_addr,
    data     => ram_data,
    cs       => ram_cs,
    oe       => ram_oe,
    we       => ram_we
  );

  clkgen: PROCESS
  BEGIN
    clk <= '1';
    WAIT FOR 10 ns;
    clk <= '0';
    WAIT FOR 10 ns;
  END PROCESS;
  
  main: PROCESS
    PROCEDURE bus_write(
        addr_i : IN STD_LOGIC_VECTOR( 19 DOWNTO 0 );
        data_i : IN STD_LOGIC_VECTOR(  7 DOWNTO 0 )
    ) IS
    BEGIN
      WAIT UNTIL clk'event AND clk = '1';
      addr <= addr_i AFTER 8.5 ns;          -- ???/8.5
      cs   <= '0' AFTER 10.8 ns;            -- 2.3/A.8
      WAIT UNTIL clk'event AND clk = '0';
      wr   <= '0' AFTER 1 ns;               -- ???/1.0
      data <= data_i AFTER 2.5 ns;          -- ???/2.5
      FOR i IN 1 TO 2 LOOP -- num waitstates
        WAIT UNTIL clk'event AND clk = '1';
        WAIT UNTIL clk'event AND clk = '0';
      END LOOP;
      WAIT UNTIL clk'event AND clk = '1';
      addr <= ( OTHERS => '0' ) AFTER 1 ns; -- 1.0/???
      data <= ( OTHERS => '0' ) AFTER 1 ns; -- 1.0/???
      cs   <= '1' AFTER 0 ns;               -- 0.0/6.0
      wr   <= '1' AFTER 0 ns;               -- 0.0/5.0
      WAIT UNTIL clk'event AND clk = '0';
    END bus_write;

    PROCEDURE bus_read(
        addr_i : IN STD_LOGIC_VECTOR( 19 DOWNTO 0 );
        data_i : IN STD_LOGIC_VECTOR(  7 DOWNTO 0 )
    ) IS
    BEGIN
      WAIT UNTIL clk'event AND clk = '1';
      addr <= addr_i AFTER 8.5 ns;          -- ???/8.5
      cs   <= '0' AFTER 8.0 ns;             -- 2.6/8.0
      rd   <= '0' AFTER 7.0 ns;             -- 2.7/7.0
      data <= ( OTHERS => 'Z' );
      FOR i IN 0 TO 2 LOOP -- num waitstates
        WAIT UNTIL clk'event AND clk = '0';
        WAIT UNTIL clk'event AND clk = '1';
      END LOOP;
      ASSERT( data = data_i );
--      data_o <= data;
      addr <= ( OTHERS => '0' ) AFTER 1 ns; -- 1.0/???
      cs   <= '1' AFTER 0 ns;               -- 0.0/6.0
      rd   <= '1' AFTER 1 ns;               -- 1.0/6.3
      WAIT UNTIL clk'event AND clk = '0';
      data <= ( OTHERS => '0' );
    END bus_read;

  BEGIN
  
    WAIT FOR 15 ns;
    bus_write( X"00000", X"04" );
    bus_write( X"00001", X"08" );
    bus_write( X"00002", X"10" );
    bus_read ( X"00004", X"00" );
    bus_read ( X"00005", X"00" );
    bus_read ( X"00006", X"00" );
    bus_write( X"00100", X"20" );
    bus_write( X"00101", X"40" );
    bus_write( X"00102", X"80" );
    page <= '1';
    bus_read ( X"00024", X"00" );
    bus_read ( X"00025", X"00" );
    bus_read ( X"00026", X"00" );
    bus_write( X"00044", X"0C" );
    bus_write( X"00045", X"30" );
    bus_write( X"00046", X"C0" );
    bus_read ( X"00004", X"00" );
    bus_read ( X"00005", X"00" );
    bus_read ( X"00006", X"00" );
    bus_read ( X"00044", X"0C" );
    bus_read ( X"00045", X"30" );
    bus_read ( X"00046", X"C0" );
  
  
    WAIT;
  END PROCESS;

END a;
