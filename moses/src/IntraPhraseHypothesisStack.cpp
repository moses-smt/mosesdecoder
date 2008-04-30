
#include "IntraPhraseHypothesisStack.h"

IntraPhraseHypothesisStack::IntraPhraseHypothesisStack(const TranslationOption &transOpt
																											 , const WordsRange &sourceRange
																											, const InputType &source
																											, const PhraseDictionary &phraseDict)
: m_sourceRange(sourceRange)
{
	SetTargetPhraseCollection(sourceRange, source, phraseDict);
	Process();
}

void IntraPhraseHypothesisStack::SetTargetPhraseCollection(const WordsRange &sourceRange
																													, const InputType &source
																													, const PhraseDictionary &phraseDict)
{
	// init size

	// create
	for (size_t startPos = sourceRange.GetStartPos() ; startPos < sourceRange.GetEndPos() ; ++startPos)
	{
		for (size_t endPos = startPos ; endPos < sourceRange.GetEndPos() ; ++endPos)
		{
			const TargetPhraseCollection *targetPhraseCollection = phraseDict.GetTargetPhraseCollection(source, WordsRange(startPos, endPos));
			SetTargetPhraseCollection(startPos, endPos, targetPhraseCollection);
		}
	}
}

void IntraPhraseHypothesisStack::SetTargetPhraseCollection(size_t startPos
																													 , size_t endPos
																													 , const TargetPhraseCollection *targetPhraseCollection)
{
	size_t startSource = m_sourceRange.GetStartPos();
	m_targetPhraseColl[startPos - startSource][endPos - startSource] = targetPhraseCollection;
}

void IntraPhraseHypothesisStack::Process()
{

}
