
#include "IntraPhraseManager.h"
#include "Util.h"

IntraPhraseManager::IntraPhraseManager(const std::vector<TranslationOption*> &transOptList
																			, const WordsRange &sourceRange
																			, const InputType &source
																			, const PhraseDictionary &phraseDict)
:m_stackColl(sourceRange.GetNumWordsCovered() + 1)
,m_sourceRange(sourceRange)
,m_tableLimit(phraseDict.GetTableLimit())
{
	float intraStackSizeMultiple = StaticData::Instance().GetIntraStackSizeMultiple();

	// get trans from dict
	SetTargetPhraseCollection(sourceRange, source, phraseDict);

	// init all stack sizes
	size_t tableLimit = phraseDict.GetTableLimit();
	for (size_t indStack = 0 ; indStack < m_stackColl.size() - 1 ; ++indStack)
	{
		m_stackColl[indStack] = new IntraPhraseHypothesisStack(tableLimit * intraStackSizeMultiple, false);
	}
	// last stack
	m_stackColl[m_stackColl.size() - 1] = new IntraPhraseHypothesisStack(tableLimit * intraStackSizeMultiple, true);

	// initialise 1st stack
	m_stackColl[0]->AddPrune(new IntraPhraseTargetPhrase(sourceRange.GetNumWordsCovered(), transOptList));

	ProcessAllStacks(true, sourceRange, transOptList);

	if (GetTargetPhraseCollection().GetSize() == 0)
	{ // didn't get any phrases. redo it without table limits
		ClearAllStacks();

		// initialise 1st stack
		m_stackColl[0]->AddPrune(new IntraPhraseTargetPhrase(sourceRange.GetNumWordsCovered(), transOptList));

		ProcessAllStacks(false, sourceRange, transOptList);
	}	
}

void IntraPhraseManager::ProcessAllStacks(bool adhereTableLimit
																					, const WordsRange &sourceRange
																					, const std::vector<TranslationOption*> &transOptList)
{
	size_t size = m_sourceRange.GetNumWordsCovered();
	for (size_t maxSegmentCount = 1; maxSegmentCount <= size; ++maxSegmentCount)
	{
		ProcessAllStacks(adhereTableLimit, maxSegmentCount);

		if (GetTargetPhraseCollection().GetSize() > 0)
		{
			return;
		}
		else
		{
			ClearAllStacks();
			
			// initialise 1st stack
			m_stackColl[0]->AddPrune(new IntraPhraseTargetPhrase(sourceRange.GetNumWordsCovered(), transOptList));
		}
	}
}

void IntraPhraseManager::ProcessAllStacks(bool adhereTableLimit, size_t maxSegmentCount)
{
	// process each stack
	vector<IntraPhraseHypothesisStack*>::iterator iterStack;
	for (iterStack = m_stackColl.begin() ; iterStack != m_stackColl.end() ; ++iterStack)
	{
		IntraPhraseHypothesisStack &stack = **iterStack;

		// go through each stack
		IntraPhraseHypothesisStack::const_iterator iterPhrase;
		for (iterPhrase = stack.begin() ; iterPhrase != stack.end() ; ++iterPhrase)
		{
			const IntraPhraseTargetPhrase &intraPhraseTargetPhrase = **iterPhrase;
			size_t segmentCount = intraPhraseTargetPhrase.GetSubRangeCount();
			if (segmentCount < maxSegmentCount)
				ProcessOnePhrase(intraPhraseTargetPhrase
												, adhereTableLimit
												, maxSegmentCount);
		}
	}

	// only keep phrases which have least num of segmentations
	m_stackColl.back()->RemoveSelectedPhrases();
}

void IntraPhraseManager::ProcessOnePhrase(const IntraPhraseTargetPhrase &intraPhraseTargetPhrase
																				, bool adhereTableLimit
																				, size_t maxSegmentCount)
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
					ExpandOnePhrase(rangeShifted
												, intraPhraseTargetPhrase
												, *targetPhraseCollection
												, adhereTableLimit
												, maxSegmentCount);
			}
		}
	}
}

void IntraPhraseManager::ExpandOnePhrase(
										const WordsRange &endSourceRange
										, const IntraPhraseTargetPhrase &intraPhraseTargetPhrase
										, const TargetPhraseCollection &targetPhraseCollection
										, bool adhereTableLimit
										, size_t maxSegmentCount)
{
	const StaticData &staticData = StaticData::Instance();

	TargetPhraseCollection::const_iterator iterPhrase, iterEnd;
	iterEnd = (!adhereTableLimit || m_tableLimit == 0 || targetPhraseCollection.GetSize() < m_tableLimit) ? targetPhraseCollection.end() : targetPhraseCollection.begin() + m_tableLimit;

	for (iterPhrase = targetPhraseCollection.begin() ; iterPhrase != iterEnd ; ++iterPhrase)
	{
		IntraPhraseTargetPhrase *newIntraPhraseTargetPhrase = new IntraPhraseTargetPhrase(intraPhraseTargetPhrase);

		const TargetPhrase &endPhrase = **iterPhrase;
		newIntraPhraseTargetPhrase->Append(endSourceRange, endPhrase);

		// check for compatibility with all previous trans opts
		if (newIntraPhraseTargetPhrase->IsCompatible())
		{
			size_t numWordsCovered = newIntraPhraseTargetPhrase->GetWordsBitmap().GetNumWordsCovered();
			IntraPhraseHypothesisStack &stack = *m_stackColl[numWordsCovered];

			if (!stack.IsLastStack() || newIntraPhraseTargetPhrase->GetSubRangeCount() == maxSegmentCount)
				stack.AddPrune(newIntraPhraseTargetPhrase);
			else
				delete newIntraPhraseTargetPhrase;
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

void IntraPhraseManager::ClearAllStacks()
{
	vector<IntraPhraseHypothesisStack*>::iterator iterStack;
	for (iterStack = m_stackColl.begin() ; iterStack != m_stackColl.end() ; ++iterStack)
	{
		IntraPhraseHypothesisStack &stack = **iterStack;
		stack.ClearAll();
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

