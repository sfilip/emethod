
tests=-2


for e in 8 ; do 
  for f in 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 52; do
		for s in 0 6  ; do
name=FPLog_$e'_'$f'_'$s'_'400
cd /home/fdedinec/Boulot/Recherche/Hard/FloPoCo.bibine/trunk
./flopoco -verbose=2 -name=$name FPLog $e $f $s TestBench $tests  
 mv flopoco.vhdl ghdl
cd ghdl
echo
echo Testing $e $f for s=$s
   ghdl -a --ieee=synopsys -fexplicit flopoco.vhdl
   ghdl -e --ieee=synopsys -fexplicit TestBench_$name
   ghdl -r --ieee=synopsys TestBench_$name --vcd=TestBench_$name.vcd
  done ;
done ;
done
 





for e in 4 5 6 ; do 
  for f in 7 8 9 10 11 12 13 14 15 16 ; do
		for s in 0 6  ; do
      name=FPLog_$e'_'$f'_'$s'_'400
      echo ./flopoco -name=$name FPLog $e $f $s
      echo ./flopoco -target=StratixII -name=$name FPLog $e $f $s
  done ;
done ;
done

name=FPLog_$e'_'$f'_'$s'_'400
cd /home/fdedinec/Boulot/Recherche/Hard/FloPoCo.bibine/trunk
./flopoco -verbose=2 -name=$name FPLog $e $f $s TestBench $tests  
 mv flopoco.vhdl ghdl
cd ghdl
echo
echo Testing $e $f for s=$s
   ghdl -a --ieee=synopsys -fexplicit flopoco.vhdl
   ghdl -e --ieee=synopsys -fexplicit TestBench_$name
   ghdl -r --ieee=synopsys TestBench_$name --vcd=TestBench_$name.vcd
  done ;
done ;
done
 



