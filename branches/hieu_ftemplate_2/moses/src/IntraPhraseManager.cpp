
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
	size_t tableLimit = phraseDict.GetTableLimit();
	for (size_t indStack = 0 ; indStack < m_stackColl.size() ; ++indStack)
	{
		m_stackColl[indStack] = new IntraPhraseHypothesisStack(tableLimit * 5);
	}

	// initialise 1st stack
	m_stackColl[0]->AddPrune(new IntraPhraseTargetPhrase(sourceRange.GetNumWordsCovered()));

	cerr << m_transOpt << endl;
	cerr << "no. of stacks " << m_stackColl.size() << endl;

	// process each stack
	vector<IntraPhraseHypothesisStack*>::iterator iterStack;
	for (iterStack = m_stackColl.begin() ; iterStack != m_stackColl.end() ; ++iterStack)
	{
		IntraPhraseHypothesisStack &stack = **iterStack;
		cerr << "size " << stack.GetSize() << endl;

		// go through each stack
		IntraPhraseHypothesisStack::const_iterator iterPhrase;
		for (iterPhrase = stack.begin() ; iterPhrase != stack.end() ; ++iterPhrase)
		{
			const IntraPhraseTargetPhrase &intraPhraseTargetPhrase = **iterPhrase;
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
					ExpandOnePhrase(rangeShifted, intraPhraseTargetPhrase, *targetPhraseCollection);
			}
		}
	}
}

void IntraPhraseManager::ExpandOnePhrase(
										const WordsRange &endSourceRange
										, const IntraPhraseTargetPhrase &intraPhraseTargetPhrase
										, const TargetPhraseCollection &targetPhraseCollection)
{
	const StaticData &staticData = StaticData::Instance();

	TargetPhraseCollection::const_iterator iterPhrase;
	for (iterPhrase = targetPhraseCollection.begin() ; iterPhrase != targetPhraseCollection.end() ; ++iterPhrase)
	{
		IntraPhraseTargetPhrase *newIntraPhraseTargetPhrase = new IntraPhraseTargetPhrase(intraPhraseTargetPhrase);

		const TargetPhrase &endPhrase = **iterPhrase;
		newIntraPhraseTargetPhrase->Append(endSourceRange, endPhrase);

		// are the overlapping factors compatible ?
		// is alignment compatible with previous trans opt ?
		bool isComp  = m_transOpt.GetTargetPhrase().IsCompatible(*newIntraPhraseTargetPhrase
																														, 0
																														, newIntraPhraseTargetPhrase->GetSize() - 1
																														, true);
		if (isComp)
		{
			// calc lm scores
			// must check for correctness
			float weightWP = staticData.GetWeightWordPenalty();
			const LMList &lmList = staticData.GetAllLM();
			newIntraPhraseTargetPhrase->RecalcLMScore(weightWP, lmList);

			size_t numWordsCovered = newIntraPhraseTargetPhrase->GetWordsBitmap().GetNumWordsCovered();
			m_stackColl[numWordsCovered]->AddPrune(newIntraPhraseTargetPhrase);
		}
		else
		{
			delete newIntraPhraseTargetPhrase;
		}
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

const IntraPhraseHypothesisStack &IntraPhraseManager::GetTargetPhraseCollection() const
{
	return *m_stackColl.back();
}

