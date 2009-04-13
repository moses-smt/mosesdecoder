
#include "IntraPhraseTargetPhrase.h"
#include "TranslationOption.h"

using namespace std;

IntraPhraseTargetPhrase::IntraPhraseTargetPhrase(size_t numWordsCovered
																								, const std::vector<TranslationOption*> &transOptList)
:m_transOptList(transOptList)
,m_sourceCompleted(numWordsCovered)
{}

void IntraPhraseTargetPhrase::Append(const WordsRange &sourceRange, const TargetPhrase &appendPhrase)
{
	m_sourceCompleted.SetValue(sourceRange.GetStartPos(), sourceRange.GetEndPos(), true);
	TargetPhrase::Append(sourceRange, appendPhrase);
}

bool IntraPhraseTargetPhrase::IsCompatible()
{
	size_t ind = 0;
	while (ind < m_transOptList.size())
	{
		const TranslationOption &transOpt = *m_transOptList[ind];
		bool isComp  = transOpt.GetTargetPhrase().IsCompatible(*this
																													, 0
																													, GetSize() - 1
																													, true);
		if (isComp)
		{ //compatible with this trans opt, do nothing
			ind++;
		}
		else
		{ // delete this trans opt
			m_transOptList.erase(m_transOptList.begin() + ind);
		}
	}

	if (m_transOptList.size() > 0)
	{ // this phrase is still usuable
		// calc lm scores. TODO must check for correctness
		const StaticData &staticData = StaticData::Instance();

		float weightWP = staticData.GetWeightWordPenalty();
		const LMList &lmList = staticData.GetAllLM();
		RecalcLMScore(weightWP, lmList);
	
		return true;
	}
	else
	{ // no prev trans opt match this phrase. discard
		return false;
	}

}

void IntraPhraseTargetPhrase::ClearOverlongTransOpts()
{
	size_t lenPhrase = GetSize();

	size_t ind = 0;
	while (ind < m_transOptList.size())
	{
		TranslationOption &transOpt = *m_transOptList[ind];
		size_t lenTransOpt = transOpt.GetTargetPhrase().GetSize();

		if (lenTransOpt != lenPhrase)
		{
			m_transOptList.erase(m_transOptList.begin() + ind);
		}
		else
		{
			ind++;
		}
	}
}


