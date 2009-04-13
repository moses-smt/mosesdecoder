

#include <cassert>
#include "TypeDef.h"
#include "Util.h"
#include "InputFileStream.h"
#include "FactorCollection.h"
#include "UnknownWordHandlerLookup.h"
#include "Word.h"

using namespace std;

UnknownWordHandlerLookup::UnknownWordHandlerLookup(FactorType sourceFactorType
																			 , FactorType targetFactorType
																			 , const std::string &filePath)
:UnknownWordHandler(sourceFactorType, targetFactorType)
{
	FactorCollection &factorCollection = FactorCollection::Instance();

	// data from file
	InputFileStream inFile(filePath);

	string line;
  size_t line_num = 0;
	while(getline(inFile, line)) 
	{
		++line_num;

		vector<string> tokens = Tokenize(line);
		assert(tokens.size() == 3);

		const Factor *source = factorCollection.AddFactor(Input, sourceFactorType, tokens[1]);
		const Factor *target = factorCollection.AddFactor(Output, targetFactorType, tokens[0]);
		float prob = Scan<float>(tokens[2]);

		UnknownWordScorePair targetPair(target, TransformScore(prob));

		m_coll[source].push_back(targetPair);
	}
}

std::list<UnknownWordScorePair> UnknownWordHandlerLookup::GetUnknownWord(const Word &sourceWord) const
{
	std::map<const Factor*, list<UnknownWordScorePair> >::const_iterator iter;

	const Factor *sourceFactor = sourceWord.GetFactor(m_sourceFactorType);
	iter = m_coll.find(sourceFactor);
	assert(iter != m_coll.end());

	const list<UnknownWordScorePair> &ret = iter->second;
	return ret;
}


