
library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;
library std;
use std.textio.all;
library work;

entity IntMultiplier_UsingDSP_8_8_16_signed_uid is
   port ( X : in  std_logic_vector(7 downto 0);
          Y : in  std_logic_vector(7 downto 0);
          R : out  std_logic_vector(15 downto 0)   );
end entity;

architecture arch of IntMultiplier_UsingDSP_8_8_16_signed_uid is
begin
   R <= X*Y;
end architecture;







library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
library std;
use std.textio.all;
library work;


entity IntMultiplier_UsingDSP_8_8_16_signed_uid2 is
   port ( X : in  std_logic_vector(7 downto 0);
          Y : in  std_logic_vector(7 downto 0);
          R : out  std_logic_vector(15 downto 0)   );
end entity;

architecture arch of IntMultiplier_UsingDSP_8_8_16_signed_uid2 is
signal XX: signed(7 downto 0);
signal YY: signed(7 downto 0);
signal RR: signed(15 downto 0);

begin
   XX <= signed(X);
   YY <= signed(Y);
   RR <= XX*YY;
   R  <= std_logic_vector(RR);
end architecture;

--------------------------------------------------------------------------------
--            TestBench_IntMultiplier_UsingDSP_8_8_16_signed_uid2
-- This operator is part of the Infinite Virtual Library FloPoCoLib
-- All rights reserved 
-- Authors: Florent de Dinechin, Cristian Klein, Nicolas Brunie (2007-2010)
--------------------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;
library std;
use std.textio.all;
library work;

entity TestBench_IntMultiplier_UsingDSP_8_8_16_signed_uid2 is
end entity;

architecture behavorial of TestBench_IntMultiplier_UsingDSP_8_8_16_signed_uid2 is
   component IntMultiplier_UsingDSP_8_8_16_signed_uid2 is
      port ( X : in  std_logic_vector(7 downto 0);
             Y : in  std_logic_vector(7 downto 0);
             R : out  std_logic_vector(15 downto 0)   );
   end component;
   signal X :  std_logic_vector(7 downto 0);
   signal Y :  std_logic_vector(7 downto 0);
   signal R :  std_logic_vector(15 downto 0);
   signal clk : std_logic;
   signal rst : std_logic;

   -- FP compare function (found vs. real)
   function fp_equal(a : std_logic_vector; b : std_logic_vector) return boolean is
   begin
      if b(b'high downto b'high-1) = "01" then
         return a = b;
      elsif b(b'high downto b'high-1) = "11" then
         return (a(a'high downto a'high-1)=b(b'high downto b'high-1));
      else
         return a(a'high downto a'high-2) = b(b'high downto b'high-2);
      end if;
   end;



 -- converts std_logic into a character
   function chr(sl: std_logic) return character is
      variable c: character;
   begin
      case sl is
         when 'U' => c:= 'U';
         when 'X' => c:= 'X';
         when '0' => c:= '0';
         when '1' => c:= '1';
         when 'Z' => c:= 'Z';
         when 'W' => c:= 'W';
         when 'L' => c:= 'L';
         when 'H' => c:= 'H';
         when '-' => c:= '-';
      end case;
      return c;
   end chr;
   -- converts bit to std_logic (1 to 1)
   function to_stdlogic(b : bit) return std_logic is
       variable sl : std_logic;
   begin
      case b is 
         when '0' => sl := '0';
         when '1' => sl := '1';
      end case;
      return sl;
   end to_stdlogic;
   -- converts std_logic into a string (1 to 1)
   function str(sl: std_logic) return string is
    variable s: string(1 to 1);
    begin
      s(1) := chr(sl);
      return s;
   end str;
   -- converts std_logic_vector into a string (binary base)
   -- (this also takes care of the fact that the range of
   --  a string is natural while a std_logic_vector may
   --  have an integer range)
   function str(slv: std_logic_vector) return string is
      variable result : string (1 to slv'length);
      variable r : integer;
   begin
      r := 1;
      for i in slv'range loop
         result(r) := chr(slv(i));
         r := r + 1;
      end loop;
      return result;
   end str;




   -- test isZero
   function iszero(a : std_logic_vector) return boolean is
   begin
      return  a = (a'high downto 0 => '0');
   end;


   -- FP IEEE compare function (found vs. real)
   function fp_equal_ieee(a : std_logic_vector; b : std_logic_vector; we : integer; wf : integer) return boolean is
   begin
      if a(wf+we downto wf) = b(wf+we downto wf) and b(we+wf-1 downto wf) = (we downto 1 => '1') then
         if iszero(b(wf-1 downto 0)) then return  iszero(a(wf-1 downto 0));
         else return not iszero(a(wf - 1 downto 0));
         end if;
      else
         return a(a'high downto 0) = b(b'high downto 0);
      end if;
   end;
begin
   test: IntMultiplier_UsingDSP_8_8_16_signed_uid2
      port map ( R => R,
                 X => X,
                 Y => Y);
   -- Ticking clock signal
   process
   begin
      clk <= '0';
      wait for 5 ns;
      clk <= '1';
      wait for 5 ns;
   end process;

   -- Setting the inputs
   process
   begin
      -- Send reset
      rst <= '1';
      wait for 10 ns;
      rst <= '0';
      X <= "00000001"; 
      Y <= "00000001"; 
      wait for 10 ns;
      X <= "11111111"; 
      Y <= "11111111"; 
      wait for 10 ns;
      X <= "10000000"; 
      Y <= "10000000"; 
      wait for 10 ns;
      X <= "00100111"; 
      Y <= "11001110"; 
      wait for 10 ns;
      X <= "11000010"; 
      Y <= "01001101"; 
      wait for 10 ns;
      X <= "01101110"; 
      Y <= "11110000"; 
      wait for 10 ns;
      X <= "01001001"; 
      Y <= "00111111"; 
      wait for 10 ns;
      X <= "00001001"; 
      Y <= "01101001"; 
      wait for 10 ns;
      X <= "10110110"; 
      Y <= "10001010"; 
      wait for 10 ns;
      X <= "01011100"; 
      Y <= "01110111"; 
      wait for 10 ns;
      X <= "10011000"; 
      Y <= "01100010"; 
      wait for 10 ns;
      X <= "11100111"; 
      Y <= "11100001"; 
      wait for 10 ns;
      X <= "11100111"; 
      Y <= "10110001"; 
      wait for 10 ns;
      X <= "01011001"; 
      Y <= "01101100"; 
      wait for 10 ns;
      X <= "11010110"; 
      Y <= "11110110"; 
      wait for 10 ns;
      X <= "01110001"; 
      Y <= "00101011"; 
      wait for 10 ns;
      X <= "10011001"; 
      Y <= "01100011"; 
      wait for 10 ns;
      X <= "10111010"; 
      Y <= "01011010"; 
      wait for 10 ns;
      X <= "00010110"; 
      Y <= "11110110"; 
      wait for 10 ns;
      wait for 100000 ns; -- allow simulation to finish
   end process;

   -- Checking the outputs
   process
   begin
      wait for 10 ns; -- wait for reset to complete
      wait for 2 ns; -- wait for pipeline to flush
      -- current time: 12
      -- input: X <= "00000001"; 
      -- input: Y <= "00000001"; 
      assert false or R="0000000000000001" report "Incorrect output value for R, expected 0000000000000001 | Test Number : 0  " severity ERROR; 
      wait for 10 ns;
      -- current time: 22
      -- input: X <= "11111111"; 
      -- input: Y <= "11111111"; 
      assert false or R="0000000000000001" report "Incorrect output value for R, expected 0000000000000001 | Test Number : 1  " severity ERROR; 
      wait for 10 ns;
      -- current time: 32
      -- input: X <= "10000000"; 
      -- input: Y <= "10000000"; 
      assert false or R="0100000000000000" report "Incorrect output value for R, expected 0100000000000000 | Test Number : 2  " severity ERROR; 
      wait for 10 ns;
      -- current time: 42
      -- input: X <= "00100111"; 
      -- input: Y <= "11001110"; 
      assert false or R="1111100001100010" report "Incorrect output value for R, expected 1111100001100010 | Test Number : 3  " severity ERROR; 
      wait for 10 ns;
      -- current time: 52
      -- input: X <= "11000010"; 
      -- input: Y <= "01001101"; 
      assert false or R="1110110101011010" report "Incorrect output value for R, expected 1110110101011010 | Test Number : 4  " severity ERROR; 
      wait for 10 ns;
      -- current time: 62
      -- input: X <= "01101110"; 
      -- input: Y <= "11110000"; 
      assert false or R="1111100100100000" report "Incorrect output value for R, expected 1111100100100000 | Test Number : 5  " severity ERROR; 
      wait for 10 ns;
      -- current time: 72
      -- input: X <= "01001001"; 
      -- input: Y <= "00111111"; 
      assert false or R="0001000111110111" report "Incorrect output value for R, expected 0001000111110111 | Test Number : 6  " severity ERROR; 
      wait for 10 ns;
      -- current time: 82
      -- input: X <= "00001001"; 
      -- input: Y <= "01101001"; 
      assert false or R="0000001110110001" report "Incorrect output value for R, expected 0000001110110001 | Test Number : 7  " severity ERROR; 
      wait for 10 ns;
      -- current time: 92
      -- input: X <= "10110110"; 
      -- input: Y <= "10001010"; 
      assert false or R="0010001000011100" report "Incorrect output value for R, expected 0010001000011100 | Test Number : 8  " severity ERROR; 
      wait for 10 ns;
      -- current time: 102
      -- input: X <= "01011100"; 
      -- input: Y <= "01110111"; 
      assert false or R="0010101011000100" report "Incorrect output value for R, expected 0010101011000100 | Test Number : 9  " severity ERROR; 
      wait for 10 ns;
      -- current time: 112
      -- input: X <= "10011000"; 
      -- input: Y <= "01100010"; 
      assert false or R="1101100000110000" report "Incorrect output value for R, expected 1101100000110000 | Test Number : 10  " severity ERROR; 
      wait for 10 ns;
      -- current time: 122
      -- input: X <= "11100111"; 
      -- input: Y <= "11100001"; 
      assert false or R="0000001100000111" report "Incorrect output value for R, expected 0000001100000111 | Test Number : 11  " severity ERROR; 
      wait for 10 ns;
      -- current time: 132
      -- input: X <= "11100111"; 
      -- input: Y <= "10110001"; 
      assert false or R="0000011110110111" report "Incorrect output value for R, expected 0000011110110111 | Test Number : 12  " severity ERROR; 
      wait for 10 ns;
      -- current time: 142
      -- input: X <= "01011001"; 
      -- input: Y <= "01101100"; 
      assert false or R="0010010110001100" report "Incorrect output value for R, expected 0010010110001100 | Test Number : 13  " severity ERROR; 
      wait for 10 ns;
      -- current time: 152
      -- input: X <= "11010110"; 
      -- input: Y <= "11110110"; 
      assert false or R="0000000110100100" report "Incorrect output value for R, expected 0000000110100100 | Test Number : 14  " severity ERROR; 
      wait for 10 ns;
      -- current time: 162
      -- input: X <= "01110001"; 
      -- input: Y <= "00101011"; 
      assert false or R="0001001011111011" report "Incorrect output value for R, expected 0001001011111011 | Test Number : 15  " severity ERROR; 
      wait for 10 ns;
      -- current time: 172
      -- input: X <= "10011001"; 
      -- input: Y <= "01100011"; 
      assert false or R="1101100000101011" report "Incorrect output value for R, expected 1101100000101011 | Test Number : 16  " severity ERROR; 
      wait for 10 ns;
      -- current time: 182
      -- input: X <= "10111010"; 
      -- input: Y <= "01011010"; 
      assert false or R="1110011101100100" report "Incorrect output value for R, expected 1110011101100100 | Test Number : 17  " severity ERROR; 
      wait for 10 ns;
      -- current time: 192
      -- input: X <= "00010110"; 
      -- input: Y <= "11110110"; 
      assert false or R="1111111100100100" report "Incorrect output value for R, expected 1111111100100100 | Test Number : 18  " severity ERROR; 
      wait for 10 ns;
      assert false report "End of simulation" severity failure;
   end process;

end architecture;

