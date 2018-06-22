#testing Shifters
#flopoco -target=Virtex4 -frequency=400 LeftShifter 40 5
#flopoco -target=Virtex4 -frequency=400 LeftShifter 40 45
#flopoco -target=Virtex4 -frequency=400 RightShifter 40 5
#flopoco -target=Virtex4 -frequency=400 RightShifter 5 45


#testing LeadingZeroCounting
flopoco -target=Virtex4 -frequency=400 LZOC 55
flopoco -target=Virtex4 -frequency=400 LZOCShifter 79 45
flopoco -target=Virtex5 -frequency=500 LZCShifter 34 45
flopoco -target=Virtex5 -frequency=500 LOCShifter 12 8
flopoco -target=Virtex5 -frequency=500 LZOCShifterSticky 79 45
flopoco -target=Virtex4 -frequency=400 LZCShifterSticky 24 18
flopoco -target=Virtex4 -frequency=400 LOCShifterSticky 22 56


#Testing IntAdder
flopoco -target=Virtex4 -frequency=400 IntAdder 1
flopoco -target=Virtex4 -frequency=400 IntAdder 16
flopoco -target=Virtex4 -frequency=400 IntAdder 128
flopoco -target=Virtex5 -frequency=500 IntAdder 128


#flexible IntAdder
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 0 0 -1 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 1 0 -1 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 2 0 -1 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 3 0 -1 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 0 1 -1 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 1 1 -1 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 2 1 -1 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 3 1 -1 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 0 0 0 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 1 0 0 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 2 0 0 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 3 0 0 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 0 1 0 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 1 1 0 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 2 1 0 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 3 1 0 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 0 0 1 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 1 0 1 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 2 0 1 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 3 0 1 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 0 1 1 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 1 1 1 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 2 1 1 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 3 1 1 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 0 0 2 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 1 0 2 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 2 0 2 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 3 0 2 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 0 1 2 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 1 1 2 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 2 1 2 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 3 1 2 0
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 0 0 -1 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 1 0 -1 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 2 0 -1 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 3 0 -1 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 0 1 -1 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 1 1 -1 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 2 1 -1 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 3 1 -1 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 0 0 0 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 1 0 0 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 2 0 0 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 3 0 0 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 0 1 0 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 1 1 0 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 2 1 0 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 3 1 0 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 0 0 1 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 1 0 1 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 2 0 1 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 3 0 1 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 0 1 1 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 1 1 1 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 2 1 1 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 3 1 1 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 0 0 2 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 1 0 2 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 2 0 2 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 3 0 2 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 0 1 2 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 1 1 2 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 2 1 2 1
#flopoco -target=Virtex4 -frequency=400 MyIntAdder 128 3 1 2 1

#intDualSub

#flopoco -frequency=500 IntDualSub 26 0
#flopoco -frequency=500 IntDualSub 26 1
#flopoco -frequency=500 IntDualSub 216 0
#flopoco -frequency=500 IntDualSub 216 1
#flopoco -frequency=500 IntDualSub 1 0
#flopoco -frequency=500 IntDualSub 1 1

#IntNAdder

#flopoco -frequency=500 IntNAdder 1 1
#flopoco -frequency=500 IntNAdder 1 2
#flopoco -frequency=500 IntNAdder 10 1
#flopoco -frequency=500 IntNAdder 100 1
#flopoco -frequency=500 IntNAdder 100 2
#flopoco -frequency=500 IntNAdder 100 10

#IntCompressorTree

#flopoco -frequency=500 IntCompressorTree 1 1
#flopoco -frequency=500 IntCompressorTree 1 2
#flopoco -frequency=500 IntCompressorTree 10 1
#flopoco -frequency=500 IntCompressorTree 100 1
#flopoco -frequency=500 IntCompressorTree 100 2
#flopoco -frequency=500 IntCompressorTree 100 10

#IntMultiplier

#flopoco -frequency=400 -target=Virtex4 IntMultiplier 10 10
#flopoco -frequency=400 -target=Virtex4 IntMultiplier 60 60
#flopoco -frequency=400 -target=Virtex5 IntMultiplier 10 10
#flopoco -frequency=400 -target=Virtex5 IntMultiplier 60 60
#flopoco -frequency=400 -target=Virtex5 IntMultiplier 10 70
#flopoco -frequency=400 -target=Virtex5 IntMultiplier 40 100

#SignedIntMultiplier

#flopoco -frequency=400 -target=Virtex4 SignedIntMultiplier 10 10
#flopoco -frequency=400 -target=Virtex4 SignedIntMultiplier 60 60
#flopoco -frequency=400 -target=Virtex5 SignedIntMultiplier 10 10
#flopoco -frequency=400 -target=Virtex5 SignedIntMultiplier 60 60
#flopoco -frequency=400 -target=Virtex5 SignedIntMultiplier 10 70
#flopoco -frequency=400 -target=Virtex5 SignedIntMultiplier 40 100

#    IntTilingMultiplier wInX wInY ratio maxTimeInMinutes

#yes | flopoco	-frequency=400 -target=Virtex4 IntTilingMultiplier 40 40 0.9 2
#yes | flopoco	-frequency=400 -target=Virtex4 IntTilingMultiplier 80 40 0.95 2
#yes | flopoco	-frequency=400 -target=Virtex5 IntTilingMultiplier 80 40 0.95 2
#yes | flopoco	-frequency=400 -target=Virtex5 IntTilingMultiplier 80 40 0.95 2


#    IntTruncMultiplier wInX wInY ratio errorOrder limit maxTimeInMinutes

#yes | flopoco	-frequency=400 -target=Virtex4 IntTruncMultiplier 40 40 0.9  40 1 2
#yes | flopoco	-frequency=400 -target=Virtex4 IntTruncMultiplier 80 40 0.95 30 1 2
#yes | flopoco	-frequency=400 -target=Virtex5 IntTruncMultiplier 80 40 0.95 60 1 2
#yes | flopoco	-frequency=400 -target=Virtex5 IntTruncMultiplier 80 40 0.95 60 1 2


#     Fix2FP LSB MSB Signed wE wF

flopoco Fix2FP -30 50 0 8 23
flopoco Fix2FP -30 50 1 8 23
flopoco Fix2FP -120 120 0 11 52


#         FPAdd wE wF

flopoco FPAdd 8 23
flopoco FPAdd 11 52
flopoco FPAdd 15 112


#         FPMult wE wF_in wF_out

flopoco FPMult 8 23 23
flopoco FPMult 8 23 52
flopoco FPMult 11 52 23


#          FPMultKaratsuba

flopoco FPMultKaratsuba 8 23 23
flopoco FPMultKaratsuba 8 23 52
flopoco FPMultKaratsuba 11 52 23


# FPMultTiling wE wF_in wF_out ratio timeInMinutes


# FPSquare wE wFin wFout

flopoco FPSquare 8 23 23
flopoco FPSquare 11 52 52


#   FPDiv wE wF

flopoco FPDiv 11 52
flopoco FPDiv 8  23

#   FPSqrt wE wF
flopoco FPSqrt 8 23
flopoco FPSqrt 11 52

#    FPSqrtPoly wE wF degree

flopoco FPSqrtPoly 8 23 2
flopoco FPSqrtPoly 11 52 5

#    IntConstMult w c
#TODO

#    FPConstMult wE_in wF_in wE_out wF_out cst_sgn cst_exp cst_int_sig
#TODO

#    FPConstMultParser wE_in wF_in wE_out wF_out wF_C constant_expr
#TODO

#    CRFPConstMult wE_in wF_in wE_out wF_out constant_expr
#TODO

#    FPLargeAcc wE_in wF_in MaxMSB_in LSB_acc MSB_acc
#TODO, no EMULATE YET
#flopoco FPLargeAcc 8 23 30 -30 50
#flopoco FPLargeAcc 11 52 50 -50 60

#    LargeAccToFP LSB_acc MSB_acc wE_out wF_out
#TODO


#   DotProduct wE wFX wFY MaxMSB_in LSB_acc MSB_acc
#TODO

#   FPExp wE wF
flopoco FPExp 8 23
flopoco FPExp 11 52

#    OutputIEEE wEI wFI wEO wFO
#      Conversion from FloPoCo to IEEE-754-like floating-point formats
#    InputIEEE wEI wFI wEO wFO
#      Conversion from IEEE-754-like to FloPoCo floating-point formats

flopoco FPLog 8 23
flopoco FPLog 11 28




