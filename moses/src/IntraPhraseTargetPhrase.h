
#pragma once

#include "TargetPhrase.h"
#include "WordsBitmap.h"

class IntraPhraseTargetPhrase: public TargetPhrase
{
protected:
	WordsBitmap				m_sourceCompleted;
public:
	IntraPhraseTargetPhrase(size_t numWordsCovered);

	const WordsBitmap &GetWordsBitmap() const
	{ return m_sourceCompleted; }
};

