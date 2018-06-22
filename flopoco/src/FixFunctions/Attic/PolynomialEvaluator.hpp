#ifndef PolynomialEvaluator_HPP
#define PolynomialEvaluator_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "../utils.hpp"

#include "../IntMultiplier.hpp"
#include "../IntAdder.hpp"
#include "../Operator.hpp"


namespace flopoco{

#define MINF -16384


	/**
	 * @brief The FixedPointCoefficinet class provides the objects for representing
	 * fixed-point coefficinets. It is used by the polynomial evaluator
	 */
	class FixedPointCoefficient
	{
	public:

		FixedPointCoefficient(FixedPointCoefficient *x);

		/**
		 * @brief Constructor. Initializes the attributes.
		 */
		FixedPointCoefficient(unsigned size, int weight);

		FixedPointCoefficient(unsigned size, int weight, mpfr_t valueMpfr);

		/** @brief Destructor */
		~FixedPointCoefficient();

		/**
		 * @brief Fetch the weight of the MSB
		 * @return weight of the MSB
		 */
		int getWeight();

		/**
		 * Set the weight of the MSB
		 */
		void setWeight(int w);


		/**
		 * @brief Fetch the size (width) of the coefficient
		 * @return coefficient width
		 */
		unsigned getSize();

		void setSize(unsigned s);


		mpfr_t* getValueMpfr();


	protected:
		int size_;        /**< The width (in bits) of the coefficient. In theory this equals to the
							 minimum number of bits required to represent abs(value) */
		int weight_;      /**< The shift value. Coefficient is represented as value * 2 (shift) */

		mpfr_t* valueMpfr_;
	};

	/**
	 * The YVar class
	 */
	class YVar: public FixedPointCoefficient
	{
	public:
		/** @brief constructor */
		YVar( unsigned size, int weight, const int sign = 0 );

		/**
		 * @brief Fetch the variable size (if known). 0 = unknown
		 * @return sign
		 */
		int getSign();

	protected:
		int sign_;        /**< The variable sign in known */
	};

	/** @brief class LevelSignal. Models The level signals */
	class LevelSignal{
	public:
		LevelSignal(string name, unsigned size, int shift);

		LevelSignal(LevelSignal* l);

		string getName();

		unsigned getSize();

		int getWeight();

	protected:
		string   name_; /**TODO Comment */
		unsigned size_;
		int      shift_;
	};

	/** @brief horner variables, sigma and pi */
	class Sigma{
	public:
		Sigma(int size1, int weight1, int size2, int weight2);

		unsigned getSize();

		int getWeight;

		int getGuardBits();

	protected:
		unsigned size;
		int      weight;
		int      guardBits;
	};

	/** @brief horner variables, sigma and pi */
	class Pi{
	public:
		Pi(unsigned size1, int weight1, unsigned size2, int weight2);

		unsigned getSize();

		int getWeight();

	protected:
		unsigned size;
		int      weight;
	};





	/**
	 * @brief The PolynomialEvaluator class. Constructs the oprator for evaluating
	 * a polynomial shiftth fix-point coefficients.
	 */
	class PolynomialEvaluator : public Operator
	{
	public:

		/**
		 * @brief The polynomial evaluator class. FIXME the parameters are subhect to change
		 * @todo document them
		 */
		PolynomialEvaluator(Target* target, vector<FixedPointCoefficient*> coef, YVar* y, int targetPrec, mpfr_t* approxError, map<string, double> inputDelays = emptyDelayMap);

		/** @brief Destructor */
		~PolynomialEvaluator();


		/* ------------------------------ EXTRA -----------------------------*/
		/* ------------------------------------------------------------------*/

		/**
		 * @brief Update coefficients.
		 * The input coefficients have the format:
		 *           size   = #of bits at the right of the .
		 *           weight = the weight of the MSB
		 * for example 0.000CCCCCC would be size=9 and weight=-3
		 * and will get converted to        size=9-3=6 (#of C) weight = -3
		 * @param coef A coefficients returned according to TableGenerator
		 */
		void updateCoefficients(vector<FixedPointCoefficient*> coef);

		/**
		 * @brief Sets the polynomial degree
		 * @param d the polynomial degree
		 */
		void setPolynomialDegree(int d);

		/** Gets the polynomial degree
		 * @return the polynomial degree
		 */
		int getPolynomialDegree();

		/**
		 * @brief print the polynomial
		 * @param[in] coef  the coefficinets
		 * @param[in] y     the variable
		 * @param[in] level the initial recursion level 0
		 * @return          the polynomial string
		 */
		string printPolynomial( vector<FixedPointCoefficient*> coef, YVar* y, int level);




		/* ----------------- ERROR RELATED ----------------------------------*/
		/* ------------------------------------------------------------------*/

		/**
		 * @brief sets the approximation error budget for the polynomial evaluator.
		 * @param p the error budget as an mpfr pointer
		 */
		void setApproximationError( mpfr_t *p);

		/**
		 * @brief sets the mpfr associated to the maximal value of y. This value
		 * is required in the error analysis step
		 * @param[in] y the variable containing info about the size and weight
		 */
		void setMaxYValue(YVar* y);

		/** @brief perform errorVector allocations */
		void allocateErrorVectors();

		/** @brief hide messy vector initializations */
		void allocateAndInitializeExplorationVectors();

		/**
		 * @brief The function that does the error estimation starting from a
		 * set of guard bits for the coefficients and a set of truncations
		 * for the Y
		 * @param[in] yGuard the vector containing the truncation error for Y
		 * @param[in] aGuard the vector with the guard bits for the coeffs.
		 * @return the maximum approximation error for this set of params
		 */
		mpfr_t* errorEstimator(vector<int> &yGuard, vector<int> &aGuard);



		void hornerGuardBitProfiler();

		/* ---------------- EXPLORATION RELATED -----------------------------*/
		/* ------------------------------------------------------------------*/

		/** @brief hide messy vector exploration vectors initializations */
		void initializeExplorationVectors();


		/** @brief fills-up a map of objective states for the truncations */
		void determineObjectiveStatesForTruncationsOnY();

		/**
		 * @brief updates the MaxBoundY vector with the maximum number of states
		 * for each Y
		 */
		void setNumberOfPossibleValuesForEachY();

		/**
		 *  @todo PROPER WAY; it only accounts (for now) on the DPS block
		 *  size. We can use as many guard bits in order not to use extra
		 *  multipliers
		 */
		void resetCoefficientGuardBits();

		void reinitCoefficientGuardBits();

		void reinitCoefficientGuardBitsLastIteration();

		/** Get range values for multiplications. We pefrom one error
		 * analysis run where we dont't truncate or add any guard bits
		 */
		void coldStart();

		/**
		 * @brief We allow possible truncation of y. Horner evaluation therefore
			allows for d differnt truncations on y (1 to d). For each y
			there are at most 3 possible truncation values:
			-no truncation
			-truncate so to fit x dimmension of the embedded multiplier
			-truncate so to fit y dimmension of the embedded multiplier
		 * @param[in] i      What y are we talking about. Takes values in 1..d
		 * @param[in] level  which one of the 3 values are we talking about 0..2
		 * @return the corresponding value to the selected level
		 */
		int getPossibleYValue(int i, int state);
		/**
		 * @brief advances to the next step in the design space exploration on the
		 * y dimmension.
		 * @return true if there is a next state, false if a solution has been
		 *found.
		 */
		bool nextStateY();

		/**
		 * @brief advances to the next step in the design space exploration on the
		 * coefficient guard bit direction.
		 * @return true if there is a next state, false if a solution has been
		 *found.
		 */
		bool nextStateA();

		unsigned getOutputSize();

		int getOutputWeight();

		vector<FixedPointCoefficient*> getCoeffParamVector();

		int getRWidth();

		int getRWeight();

	protected:

		mpfr_t* approximationError; /* the error performed in the polynomial finding step */
		mpfr_t targetError;         /* the target error for this component. In most cases
									   this should be equal to 1/2 ulp as the other 1/2 ulp
									   is lost in the final rounding */

		unsigned wR;
		int weightR;

		vector<FixedPointCoefficient*> coef_;
		YVar* y_;
		int targetPrec_;
		unsigned degree_;
		mpfr_t maxABSy;



		vector<int> yGuard_; // negative values => truncation
		vector<int> yState_;


		vector<int> aGuard_; // positive values refer to "real" guard bits

		vector<int> maxBoundY;
		vector<int> maxBoundA;


		int currentPrec;
		bool sol;

		float thresholdForDSP;

		vector<signed> sigmakPSize,  pikPSize, pikPTSize;
		vector<signed> sigmakPWeight, pikPWeight, pikPTWeight;

	private:
		multimap<int, int> objectiveStatesY;

		/* for efficiency we move these variables here */
	};
}

#endif
