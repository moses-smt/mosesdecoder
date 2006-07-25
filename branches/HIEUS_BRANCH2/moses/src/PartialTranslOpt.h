
#pragma once


class PartialTranslOpt
{
protected:
	const WordsRange		m_sourceWordRange;
	const TargetPhrase	m_targetPhrase;
public:
	PartialTranslOpt(const WordsRange &sourceWordRange, const TargetPhrase &targetPhrase)
		:m_sourceWordRange(sourceWordRange)
		,m_targetPhrase(targetPhrase)
	{
	}

	const WordsRange &GetSourceWordsRange() const
	{
		return m_sourceWordRange;
	}
	const TargetPhrase &GetTargetPhrase() const
	{
		return m_targetPhrase;
	}
};

