
#pragma once

#include <set>
#include <vector>
#include "IntraPhraseTargetPhrase.h"
#include "TranslationOption.h"
#include "WordsRange.h"
#include "InputType.h"
#include "TargetPhraseCollection.h"

class IntraPhraseHypothesisStack 
{
private:
	typedef std::set<IntraPhraseTargetPhrase> CollType;
	CollType m_coll;

	std::vector<std::vector<const TargetPhraseCollection*> > m_targetPhraseColl;
	const WordsRange &m_sourceRange;

	void Process();
	
	void SetTargetPhraseCollection(const WordsRange &sourceRange
																, const InputType &source
																, const PhraseDictionary &phraseDict);
	void SetTargetPhraseCollection(size_t startPos
																, size_t endPos
																, const TargetPhraseCollection *targetPhraseCollection);

public:
	IntraPhraseHypothesisStack(const TranslationOption &transOpt, const WordsRange &sourceRange
													, const InputType &source, const PhraseDictionary &phraseDict);

};

