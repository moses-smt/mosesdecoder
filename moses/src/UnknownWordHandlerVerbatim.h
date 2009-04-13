
#pragma once

#include "UnknownWordHandler.h"

class UnknownWordHandlerVerbatim: public UnknownWordHandler
{
public:
	UnknownWordHandlerVerbatim(FactorType sourceFactorType, FactorType targetFactorType);
	std::list<UnknownWordScorePair> GetUnknownWord(const Word &sourceWord) const;


};

