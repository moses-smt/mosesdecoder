
#pragma once

#include <string>
#include <list>
#include "TypeDef.h"

class Factor;
class Word;

typedef std::pair<const Factor*, float> UnknownWordScorePair;

class UnknownWordHandler
{
protected:
	FactorType m_sourceFactorType, m_targetFactorType;

	UnknownWordHandler(FactorType sourceFactorType, FactorType targetFactorType)
			:m_sourceFactorType(sourceFactorType)
			,m_targetFactorType(targetFactorType)
	{}
public:
	virtual ~UnknownWordHandler()
	{}
	virtual std::list<UnknownWordScorePair> GetUnknownWord(const Word &sourceWord) const = 0;
};

