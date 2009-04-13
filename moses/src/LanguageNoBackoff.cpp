
#include "LanguageNoBackoff.h"
#include "FactorCollection.h"
#include "NGramNode.h"
#include "InputFileStream.h"
#include "StaticData.h"

using namespace std;

LanguageNoBackoff::LanguageNoBackoff(bool registerScore, ScoreIndexManager &scoreIndexManager)
:LanguageModelSingleFactor(registerScore, scoreIndexManager)
{
}

bool LanguageNoBackoff::Load(const std::string &filePath
																, FactorType factorType
																, float weight
																, size_t nGramOrder)
{
	VERBOSE(1, "Loading No Backoff LM: " << filePath << endl);
	
	FactorCollection &factorCollection = FactorCollection::Instance();

	m_filePath		= filePath;
	m_factorType	= factorType;
	m_weight			= weight;
	m_nGramOrder	= nGramOrder;

	// make sure start & end tags in factor collection
	m_sentenceStart	= factorCollection.AddFactor(Output, m_factorType, BOS_);
	m_sentenceStartArray[m_factorType] = m_sentenceStart;

	m_sentenceEnd		= factorCollection.AddFactor(Output, m_factorType, EOS_);
	m_sentenceEndArray[m_factorType] = m_sentenceEnd;

	// read in file
	VERBOSE(1, filePath << endl);

	InputFileStream 	inFile(filePath);

	// to create lookup vector later on
	size_t maxFactorId = 0; 
	map<size_t, const NGramNode*> lmIdMap;

	string line;
	int lineNo = 0;
	
	while( !getline(inFile, line, '\n').eof())
	{
		lineNo++;

		if (line.size() != 0 && line.substr(0,1) != "\\")
		{
			vector<string> tokens = Tokenize(line, "\t");
			if (tokens.size() >= 2)
			{
				// split unigram/bigram trigrams
				vector<string> factorStr = Tokenize(tokens[1], " ");

				// create / traverse down tree
				NGramCollection *ngramColl = &m_map;
				NGramNode *nGram;
				const Factor *factor;
				for (int currFactor = (int) factorStr.size() - 1 ; currFactor >= 0  ; currFactor--)
				{
					factor = factorCollection.AddFactor(Output, m_factorType, factorStr[currFactor]);
					nGram = ngramColl->GetOrCreateNGram(factor);
	
					ngramColl = nGram->GetNGramColl();

				}

				NGramNode *rootNGram = m_map.GetNGram(factor);
				nGram->SetRootNGram(rootNGram);

				// create vector of factors used in this LM
				size_t factorId = factor->GetId();
				maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
				lmIdMap[factorId] = rootNGram;
				//factorCollection.SetFactorLmId(factor, rootNGram);

				float score = TransformSRIScore(Scan<float>(tokens[0]));
				nGram->SetScore( score );
				if (tokens.size() == 3)
				{
					float logBackOff = TransformSRIScore(Scan<float>(tokens[2]));
					nGram->SetLogBackOff( logBackOff );
				}
				else
				{
					nGram->SetLogBackOff( 0 );
				}
			}
		}
	}

		// add to lookup vector in object
	m_lmIdLookup.resize(maxFactorId+1);
	fill(m_lmIdLookup.begin(), m_lmIdLookup.end(), static_cast<const NGramNode*>(NULL));

	map<size_t, const NGramNode*>::iterator iterMap;
	for (iterMap = lmIdMap.begin() ; iterMap != lmIdMap.end() ; ++iterMap)
	{
		m_lmIdLookup[iterMap->first] = iterMap->second;
	}

	return true;
}

float LanguageNoBackoff::GetValue(const std::vector<const Word*> &contextFactor
												, State* finalState
												, unsigned int* len) const
{
	const size_t order = contextFactor.size();

	// 1st word
	const Word &word = *contextFactor[0];
	const NGramNode *nGram		= GetLmID(word[m_factorType]);
	
	for (size_t pos = 1 ; pos < order && nGram != NULL ; ++pos)
	{
		const Word &word = *contextFactor[pos];
		nGram = nGram->GetNGram(word[m_factorType]);
	}

	if (finalState != NULL)
		*finalState = static_cast<const void*>(nGram);

	return (nGram==NULL)?-100:nGram->GetScore();
}

