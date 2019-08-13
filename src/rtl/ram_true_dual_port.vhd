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
-- This is a true dual-port RAM implementation that should infer block RAM:s in FPGA:s.
-- Inspired by: https://danstrother.com/2010/09/11/inferring-rams-in-fpgas/
----------------------------------------------------------------------------------------------------
 
library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
 
entity ram_true_dual_port is
  generic(
    DATA_BITS : integer := 72;
    ADR_BITS : integer := 10
  );
  port(
    -- Port A
    i_clk_a : in std_logic;
    i_we_a : in std_logic;
    i_adr_a : in std_logic_vector(ADR_BITS-1 downto 0);
    i_data_a : in std_logic_vector(DATA_BITS-1 downto 0);
    o_data_a : out std_logic_vector(DATA_BITS-1 downto 0);
     
    -- Port B
    i_clk_b : in std_logic;
    i_we_b : in std_logic;
    i_adr_b : in std_logic_vector(ADR_BITS-1 downto 0);
    i_data_b : in std_logic_vector(DATA_BITS-1 downto 0);
    o_data_b : out std_logic_vector(DATA_BITS-1 downto 0)
  );
end ram_true_dual_port;
 
architecture rtl of ram_true_dual_port is
  type T_MEM is array ((2**ADR_BITS)-1 downto 0) of std_logic_vector(DATA_BITS-1 downto 0);
  shared variable v_mem : T_MEM;
begin
  -- Port A
  process(i_clk_a)
  begin
    if i_clk_a'event and i_clk_a = '1' then
      if i_we_a = '1' then
        v_mem(conv_integer(i_adr_a)) := i_data_a;
      end if;
      o_data_a <= v_mem(conv_integer(i_adr_a));
    end if;
  end process;
 
  -- Port B
  process(i_clk_b)
  begin
    if i_clk_b'event and i_clk_b = '1' then
      if i_we_b = '1' then
        v_mem(conv_integer(i_adr_b)) := i_data_b;
      end if;
      o_data_b <= v_mem(conv_integer(i_adr_b));
    end if;
  end process;
end rtl;