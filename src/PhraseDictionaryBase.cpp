#include "PhraseDictionaryBase.h"
#include "StaticData.h"
#include "InputType.h"

PhraseDictionaryBase::PhraseDictionaryBase(size_t noScoreComponent)
	: Dictionary(noScoreComponent),m_tableLimit(0)
{
	const_cast<ScoreIndexManager&>(StaticData::Instance()->GetScoreIndexManager()).AddScoreProducer(this);
}

PhraseDictionaryBase::~PhraseDictionaryBase() {}
	
const TargetPhraseCollection *PhraseDictionaryBase::
GetTargetPhraseCollection(InputType const& src,WordsRange const& range) const 
{
	return GetTargetPhraseCollection(src.GetSubString(range));
}

const std::string PhraseDictionaryBase::GetScoreProducerDescription() const
{
	return "Translation score, file=" + m_filename;
}

unsigned int PhraseDictionaryBase::GetNumScoreComponents() const
{
	return this->GetNoScoreComponents();
}
