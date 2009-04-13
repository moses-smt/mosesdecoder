
#pragma once

#include <vector>
#include "TargetPhrase.h"
#include "WordsBitmap.h"

class TranslationOption;

class IntraPhraseTargetPhrase: public TargetPhrase
{
protected:
	std::vector<TranslationOption*> m_transOptList;
	WordsBitmap				m_sourceCompleted;
public:
	IntraPhraseTargetPhrase(size_t numWordsCovered
												, const std::vector<TranslationOption*> &transOptList);

	const WordsBitmap &GetWordsBitmap() const
	{ return m_sourceCompleted; }

	void Append(const WordsRange &sourceRange, const TargetPhrase &appendPhrase);

	bool IsCompatible();
	void ClearOverlongTransOpts();
	
	const std::vector<TranslationOption*> &GetTranslationOptionList() const
	{ return m_transOptList; }
};

