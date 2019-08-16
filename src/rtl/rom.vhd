----------------------------------------------------------------------------------------------------
-- Copyright (c) 2019 Marcus Geelnard
--
-- This software is provided 'as-is', without any express or implied warranty. In no event will the
-- authors be held liable for any damages arising from the use of this software.
--
-- Permission is granted to anyone to use this software for any purpose, including commercial
-- applications, and to alter it and redistribute it freely, subject to the following restrictions:
--
--  1. The origin of this software must not be misrepresented; you must not claim that you wrote
--     the original software. If you use this software in a product, an acknowledgment in the
--     product documentation would be appreciated but is not required.
--
--  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
--     being the original software.
--
--  3. This notice may not be removed or altered from any source distribution.
----------------------------------------------------------------------------------------------------

----------------------------------------------------------------------------------------------------
-- This is a single-ported ROM (Wishbone B4 pipelined interface).
----------------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity rom is
  port(
    -- Control signals.
    i_clk : in std_logic;

    -- Wishbone memory interface (b4 pipelined slave).
    -- See: https://cdn.opencores.org/downloads/wbspec_b4.pdf
    i_wb_cyc : in std_logic;
    i_wb_stb : in std_logic;
    i_wb_adr : in std_logic_vector(31 downto 2);
    o_wb_dat : out std_logic_vector(31 downto 0);
    o_wb_ack : out std_logic;
    o_wb_stall : out std_logic
  );
end rom;

architecture rtl of rom is
  constant C_ADDR_BITS : positive := 8;
  constant C_ROM_SIZE : positive := 2**C_ADDR_BITS;

  type T_ROM_ARRAY is array (0 to C_ROM_SIZE-1) of std_logic_vector(31 downto 0);
  constant C_ROM_ARRAY : T_ROM_ARRAY := (
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"e8200000",
    x"e8400000",
    x"e8600000",
    x"e8800000",
    x"e8a00000",
    x"e8c00000",
    x"e8e00000",
    x"e9000000",
    x"e9200000",
    x"e9400000",
    x"e9600000",
    x"e9800000",
    x"e9a00000",
    x"e9c00000",
    x"e9e00000",
    x"ea000000",
    x"ea200000",
    x"ea400000",
    x"ea600000",
    x"ea800000",
    x"eaa00000",
    x"eac00000",
    x"eae00000",
    x"eb000000",
    x"eb200000",
    x"eb400000",
    x"eb600000",
    x"eb800000",
    x"eba00000",
    x"ebc00000",
    x"03a00000",
    x"40208000",
    x"40408000",
    x"40608000",
    x"40808000",
    x"40a08000",
    x"40c08000",
    x"40e08000",
    x"41008000",
    x"41208000",
    x"41408000",
    x"41608000",
    x"41808000",
    x"41a08000",
    x"41c08000",
    x"41e08000",
    x"42008000",
    x"42208000",
    x"42408000",
    x"42608000",
    x"42808000",
    x"42a08000",
    x"42c08000",
    x"42e08000",
    x"43008000",
    x"43208000",
    x"43408000",
    x"43608000",
    x"43808000",
    x"43a08000",
    x"43c08000",
    x"43e08000",
    x"eba00000",
    x"40208000",
    x"40408000",
    x"40608000",
    x"40808000",
    x"40a08000",
    x"40c08000",
    x"40e08000",
    x"41008000",
    x"41208000",
    x"41408000",
    x"41608000",
    x"41808000",
    x"41a08000",
    x"41c08000",
    x"41e08000",
    x"42008000",
    x"42208000",
    x"42408000",
    x"42608000",
    x"42808000",
    x"42a08000",
    x"42c08000",
    x"42e08000",
    x"43008000",
    x"43208000",
    x"43408000",
    x"43608000",
    x"43808000",
    x"43a08000",
    x"43c08000",
    x"43e08000",
    x"03a00000",
    x"ef880080",
    x"579c0000",
    x"e8200001",
    x"ec400000",
    x"544203c8",
    x"ede00000",
    x"e5e000f1",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"e3e00000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"e3c00000",
    x"000003cc",
    x"676f7270",
    x"006d6172",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000",
    x"00000000"
  );

  signal s_is_valid_wb_request : std_logic;
begin
  -- Wishbone control logic.
  s_is_valid_wb_request <= i_wb_cyc and i_wb_stb;

  -- We always ack and never stall - we're that fast ;-)
  process(i_clk)
  begin
    if rising_edge(i_clk) then
      o_wb_dat <= C_ROM_ARRAY(to_integer(unsigned(i_wb_adr(C_ADDR_BITS+1 downto 2))));
      o_wb_ack <= s_is_valid_wb_request;
    end if;
  end process;
  o_wb_stall <= '0';
end rtl;
