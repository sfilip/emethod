
ghdl -a --ieee=standard --ieee=synopsys -fexplicit testSigned.vhdl
ghdl -e --ieee=standard --ieee=synopsys -fexplicit TestBench_IntMultiplier_UsingDSP_8_8_16_signed_uid2
ghdl -r --ieee=standard --ieee=synopsys -fexplicit TestBench_IntMultiplier_UsingDSP_8_8_16_signed_uid2 --vcd=TestBench_IntMultiplier_UsingDSP_8_8_16_signed_uid2.vcd
