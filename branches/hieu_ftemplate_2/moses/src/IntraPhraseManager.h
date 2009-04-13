
#pragma once

#include <vector>
#include "IntraPhraseHypothesisStack.h"
#include "TranslationOption.h"
#include "WordsRange.h"
#include "InputType.h"

class TargetPhraseCollection;

class IntraPhraseManager
{
protected:
	std::vector<IntraPhraseHypothesisStack*> m_stackColl;
	std::vector<std::vector<const TargetPhraseCollection*> > m_targetPhraseColl;
	const WordsRange &m_sourceRange;
	const size_t m_tableLimit;

	void SetTargetPhraseCollection(const WordsRange &sourceRange
																, const InputType &source
																, const PhraseDictionary &phraseDict);
	void SetTargetPhraseCollection(size_t startPos
																, size_t endPos
																, const TargetPhraseCollection *targetPhraseCollection);
	const TargetPhraseCollection *GetTargetPhraseCollection(size_t startPos
																												, size_t endPos) const;
	void ProcessAllStacks(bool adhereTableLimit, size_t maxSegmentCount);
	void ProcessAllStacks(bool adhereTableLimit
											, const WordsRange &sourceRange
											, const std::vector<TranslationOption*> &transOptList);
	void ProcessOnePhrase(const IntraPhraseTargetPhrase &intraPhraseTargetPhrase
											, bool adhereTableLimit
											, size_t maxSegmentCount);
	void ExpandOnePhrase(const WordsRange &endSourceRange
											, const IntraPhraseTargetPhrase &intraPhraseTargetPhrase
											, const TargetPhraseCollection &targetPhraseCollection
											, bool adhereTableLimit
											, size_t maxSegmentCount);
	void ClearAllStacks();
	
public:
	IntraPhraseManager(const std::vector<TranslationOption*> &transOptList
										, const WordsRange &sourceRange
										, const InputType &source
										, const PhraseDictionary &phraseDict);
	~IntraPhraseManager();

	const IntraPhraseHypothesisStack &GetTargetPhraseCollection() const;
};

