
#include "IntraPhraseManager.h"
#include "Util.h"

IntraPhraseManager::IntraPhraseManager(const TranslationOption &transOpt
																			, const WordsRange &sourceRange
																			, const InputType &source
																			, const PhraseDictionary &phraseDict)
:m_transOpt(transOpt)
,m_stackColl(sourceRange.GetNumWordsCovered() + 1)
,m_sourceRange(sourceRange)
{
	// get trans from dict
	SetTargetPhraseCollection(sourceRange, source, phraseDict);

	// init all stack sizes
	for (size_t indStack = 0 ; indStack < m_stackColl.size() ; ++indStack)
	{
		m_stackColl[indStack] = new IntraPhraseHypothesisStack(20);
	}

	// initialise 1st stack
	m_stackColl[0]->AddPrune(IntraPhraseTargetPhrase(sourceRange.GetNumWordsCovered()));

	// process each stack
	vector<IntraPhraseHypothesisStack*>::iterator iterStack;
	for (iterStack = m_stackColl.begin() ; iterStack != m_stackColl.end() ; ++iterStack)
	{
		IntraPhraseHypothesisStack &stack = **iterStack;
		// go through each stack
		IntraPhraseHypothesisStack::const_iterator iterPhrase;
		for (iterPhrase = stack.begin() ; iterPhrase != stack.end() ; ++iterPhrase)
		{
			const IntraPhraseTargetPhrase &intraPhraseTargetPhrase = *iterPhrase;
			ProcessOnePhrase(intraPhraseTargetPhrase);
		}
	}
}

void IntraPhraseManager::ProcessOnePhrase(const IntraPhraseTargetPhrase &intraPhraseTargetPhrase)
{
	size_t startSource = m_sourceRange.GetStartPos();
	for (size_t startPos = m_sourceRange.GetStartPos() ; startPos <= m_sourceRange.GetEndPos() ; ++startPos)
	{
		for (size_t endPos = startPos ; endPos <= m_sourceRange.GetEndPos() ; ++endPos)
		{
			WordsRange rangeShifted(startPos - startSource, endPos - startSource);
			
			if (!intraPhraseTargetPhrase.GetWordsBitmap().Overlap(rangeShifted))
			{ // expand phrase
				const TargetPhraseCollection *targetPhraseCollection = GetTargetPhraseCollection(startPos, endPos);
				if (targetPhraseCollection != NULL)
					ExpandOnePhrase(WordsRange(startPos, endPos), intraPhraseTargetPhrase, *targetPhraseCollection);
			}
		}
	}
}

void IntraPhraseManager::ExpandOnePhrase(
										const WordsRange &endSourceRange
										, const IntraPhraseTargetPhrase &intraPhraseTargetPhrase
										, const TargetPhraseCollection &targetPhraseCollection)
{
	cerr << m_transOpt << endl;

	TargetPhraseCollection::const_iterator iterPhrase;
	for (iterPhrase = targetPhraseCollection.begin() ; iterPhrase != targetPhraseCollection.end() ; ++iterPhrase)
	{
		IntraPhraseTargetPhrase newIntraPhraseTargetPhrase(intraPhraseTargetPhrase);
		cerr << newIntraPhraseTargetPhrase << endl;

		const TargetPhrase &endPhrase = **iterPhrase;
		newIntraPhraseTargetPhrase.Append(endSourceRange, endPhrase);
		cerr << newIntraPhraseTargetPhrase << endl;

		// are the overlapping factors compatible ?
		// is alignment compatible with previous trans opt ?
		bool isComp  = m_transOpt.GetTargetPhrase().IsCompatible(newIntraPhraseTargetPhrase
																														, 0
																														, newIntraPhraseTargetPhrase.GetSize());

		/*
		isComp = m_transOpt.GetAlignmentPair().IsCompatible(
																									newIntraPhraseTargetPhrase.GetAlignmentPair()
																									, 0
																									, 0);
		*/

		// calc lm scores
		// must check for correctness
		const StaticData &staticData = StaticData::Instance();
		float weightWP = staticData.GetWeightWordPenalty();
		const LMList &lmList = staticData.GetAllLM();
		newIntraPhraseTargetPhrase.RecalcLMScore(weightWP, lmList);
	} // for (iterPhrase 
}

void IntraPhraseManager::SetTargetPhraseCollection(const WordsRange &sourceRange
																													, const InputType &source
																													, const PhraseDictionary &phraseDict)
{
	// init size
	size_t numCovered = sourceRange.GetNumWordsCovered();
	m_targetPhraseColl.resize(numCovered);

	size_t startSource = m_sourceRange.GetStartPos();
	size_t otherSize = numCovered;
	for (size_t startPos = sourceRange.GetStartPos() ; startPos <= sourceRange.GetEndPos() ; ++startPos)
	{
		m_targetPhraseColl[startPos - startSource].resize(otherSize);
		otherSize--;
	}

	// create half matrix of target phrase coll from dict
	for (size_t startPos = sourceRange.GetStartPos() ; startPos <= sourceRange.GetEndPos() ; ++startPos)
	{
		for (size_t endPos = startPos ; endPos <= sourceRange.GetEndPos() ; ++endPos)
		{
			const TargetPhraseCollection *targetPhraseCollection = phraseDict.GetTargetPhraseCollection(source, WordsRange(startPos, endPos));
			SetTargetPhraseCollection(startPos, endPos, targetPhraseCollection);
		}
	}
}

IntraPhraseManager::~IntraPhraseManager()
{
	RemoveAllInColl(m_stackColl);
}

void IntraPhraseManager::SetTargetPhraseCollection(size_t startPos
																													 , size_t endPos
																													 , const TargetPhraseCollection *targetPhraseCollection)
{
	size_t startSource = m_sourceRange.GetStartPos();
	m_targetPhraseColl[startPos - startSource][endPos - startPos] = targetPhraseCollection;
}

const TargetPhraseCollection *IntraPhraseManager::GetTargetPhraseCollection(size_t startPos
																																					, size_t endPos) const
{
	size_t startSource = m_sourceRange.GetStartPos();
	return m_targetPhraseColl[startPos - startSource][endPos - startPos];
}


