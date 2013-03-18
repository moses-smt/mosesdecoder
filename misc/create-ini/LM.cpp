#include "LM.h"

int LM::s_index = 0;

void LM::Output(std::ostream &out) const
{
    out << name
	<< " name=LM" << index
        << " order=" << order
        << " factor=" << OutputFactors(outFactors)
        << " path=" << path
        << " " << otherArgs
        << std::endl;
}

void LM::OutputWeights(std::ostream &out) const
{
    out << "LM" << index << "= ";
    for (size_t i = 0; i < numFeatures; ++i) {
	out << GetWeight() << " ";
    }
    out << std::endl;
}
