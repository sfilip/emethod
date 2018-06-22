
FixFunction
    Centralizes Sollya parsing and evaluation to various formats, including an emulate() that works for all FixFunctionBy*

FixFunctionByTable: a flopoco operator that is the most basic implementation of a FixFunction




PiecewisePolyApprox: Iterates over the previous to build a regular piecewise polynomial approximation. Roughly the approximation part of previous FunctionEvaluator
DONE



FixPolynomialHornerEvaluator: 
  constructor inputs alpha,  a table of 2^alpha vectors of coefficients, lsbY
  and builds table + horner evaluator (operator inputting index and Y)

			evaluates the corresponding polynomial(s) with truncated multipliers.
								REM a trunc mult faithful to lsb will compute weights to LSB+g 



FixFunctionBySimplePoly: a flopoco operator plugging  BasicPolyApprox to FixPolynomialHornerEvaluator
TODO we should get rid of this and replace it with a new constructor of FixFunctionByPiecewisePoly



FixFunctionBySimpleBitHeap: an Operator plugging BasicPolyApprox to a bit heap back-end.
													 (with bit heaps you don't do piecewise polynomials, or you do it the HOTBM way)


FixFunctionByPiecewisePoly:
								the flopoco operator corresponding to the previous FunctionEvaluator
								It plugs PiecewisePolyApprox to FixPolynomialHornerEvaluator


				
GenericEvaluator:
	A wrapper class written by Sylvain that should be able to instantiate any of the others.
	Still in the src but currently disabled, and to be renamed.





The attic directory is full of old stuff.

TODO short term: 
Probably need to add signed/unsigned IOs to IntMultiplier (?FixMultiplier?) -- prepare discussion
Add signed/unsigned support to the test framework (IO conversions from/to std_logic_vector).


TODO long term: manage non-uniform errors; manage non-uniform segmentation; something for sin(sqrt(x))


