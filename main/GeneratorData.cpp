
#include "GeneratorData.hpp"

namespace emethod {

	GeneratorData::GeneratorData(
			string fStr_,
			string wStr_,
			string deltaStr_,
			int scalingFactor_,
			int r_,
			int lsbInOut_,
			string xiStr_,
			string alphaStr_,
			int msbInOut_,
			bool scaleInput_,
			string inputScalingFactorStr_,
			int verbosity_,
			bool isPipelined_,
			int frequency_,
			int nbTests_):
		r(r_), lsbInOut(lsbInOut_), msbInOut(msbInOut_),
		scaleInput(scaleInput_),
		verbosity(verbosity_), isPipelined(isPipelined_), frequency(frequency_), nbTests(nbTests_)
	{
		ftokens.clear();
		ftokens = tokenizer(fStr_).getTokens();
		f = [this](mpreal var) -> mpreal
		{
			return this->sh.evaluate(this->ftokens, var);
		};

		wtokens.clear();
		wtokens = tokenizer(wStr_).getTokens();
		w = [this](mpreal var) -> mpreal
		{
			return this->sh.evaluate(this->wtokens, var);
		};

		delta = mpreal(deltaStr_);
		scalingFactor = mpreal(1) << scalingFactor_;

		xi = mpreal(xiStr_);
		alpha = mpreal(alphaStr_);

		type = make_pair(4, 4);
		dom = make_pair(mpreal(0), mpreal(1.0 / 32));
		d2 = alpha - max(abs(dom.first), abs(dom.second));
		d1 = -d2;

		inputScalingFactor = mpreal(inputScalingFactorStr_);
	}


	GeneratorData::~GeneratorData() {
		// TODO Auto-generated destructor stub
	}

} /* namespace emethod */
