
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
	const TranslationOption &m_transOpt;
	const WordsRange &m_sourceRange;

	void SetTargetPhraseCollection(const WordsRange &sourceRange
																, const InputType &source
																, const PhraseDictionary &phraseDict);
	void SetTargetPhraseCollection(size_t startPos
																, size_t endPos
																, const TargetPhraseCollection *targetPhraseCollection);
	const TargetPhraseCollection *GetTargetPhraseCollection(size_t startPos
																												, size_t endPos) const;
	void ProcessOnePhrase(const IntraPhraseTargetPhrase &intraPhraseTargetPhrase);
	void ExpandOnePhrase(const WordsRange &endSourceRange
											, const IntraPhraseTargetPhrase &intraPhraseTargetPhrase
											, const TargetPhraseCollection &targetPhraseCollection);

public:
	IntraPhraseManager(const TranslationOption &transOpt
										, const WordsRange &sourceRange
										, const InputType &source
										, const PhraseDictionary &phraseDict);
	~IntraPhraseManager();

	const IntraPhraseHypothesisStack &GetTargetPhraseCollection() const;
};

