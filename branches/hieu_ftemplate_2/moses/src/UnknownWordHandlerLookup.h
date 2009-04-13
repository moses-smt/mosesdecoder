
#pragma once

#include <map>
#include "UnknownWordHandler.h"

class UnknownWordHandlerLookup: public UnknownWordHandler
{
protected:
	std::map<const Factor*, std::list<UnknownWordScorePair> > m_coll;

public:
	UnknownWordHandlerLookup(FactorType sourceFactorType, FactorType targetFactorType
										,const std::string &filePath);
	
	std::list<UnknownWordScorePair> GetUnknownWord(const Word &sourceWord) const;

};
