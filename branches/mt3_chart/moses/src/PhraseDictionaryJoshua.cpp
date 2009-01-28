
#include "PhraseDictionaryJoshua.h"

namespace Moses
{

PhraseDictionaryJoshua::~PhraseDictionaryJoshua()
{

}

bool PhraseDictionaryJoshua::Load(const std::vector<FactorType> &input
								, const std::vector<FactorType> &output
								, const std::string &filePath
								, const std::vector<float> &weight
								, size_t tableLimit)
{


	return true;
}

void PhraseDictionaryJoshua::SetWeightTransModel(const std::vector<float> &weightT)
{
}

void PhraseDictionaryJoshua::CleanUp()
{
}

const TargetPhraseCollection *PhraseDictionaryJoshua::GetTargetPhraseCollection(const Phrase& src) const
{

	return NULL;
}

void PhraseDictionaryJoshua::AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase)
{

}

const ChartRuleCollection *PhraseDictionaryJoshua::GetChartRuleCollection(
																				InputType const& src
																				,WordsRange const& range
																				,bool adhereTableLimit) const
{

	return NULL;
}


} // namespace

