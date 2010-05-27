
#include "LanguageModelInternal.h"
#include "FactorCollection.h"
#include "NGramNode.h"
#include "InputFileStream.h"
#include "StaticData.h"
#include "UserMessage.h"

using namespace std;

namespace Moses
{
LanguageModelInternal::LanguageModelInternal(bool registerScore, ScoreIndexManager &scoreIndexManager)
:LanguageModelSingleFactor(registerScore, scoreIndexManager)
{
}

bool LanguageModelInternal::Load(const std::string &filePath
																, FactorType factorType
																, float weight
																, size_t nGramOrder)
{
	assert(nGramOrder <= 3);
	if (nGramOrder > 3)
	{
		UserMessage::Add("Can only do up to trigram. Aborting");
		abort();
	}

	VERBOSE(1, "Loading Internal LM: " << filePath << endl);
	
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

				float score = TransformLMScore(Scan<float>(tokens[0]));
				nGram->SetScore( score );
				if (tokens.size() == 3)
				{
					float logBackOff = TransformLMScore(Scan<float>(tokens[2]));
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

float LanguageModelInternal::GetValue(const std::vector<const Word*> &contextFactor
												, State* finalState
												, unsigned int* /*len*/) const
{
	const size_t ngram = contextFactor.size();
	switch (ngram)
	{
	case 1: return GetValue((*contextFactor[0])[m_factorType], finalState); break;
	case 2: return GetValue((*contextFactor[0])[m_factorType]
												, (*contextFactor[1])[m_factorType], finalState); break;
	case 3: return GetValue((*contextFactor[0])[m_factorType]
												, (*contextFactor[1])[m_factorType]
												, (*contextFactor[2])[m_factorType], finalState); break;
	}

	assert (false);
	return 0;
}

float LanguageModelInternal::GetValue(const Factor *factor0, State* finalState) const
{
	float prob;
	const NGramNode *nGram		= GetLmID(factor0);
	if (nGram == NULL)
	{
		if (finalState != NULL)
			*finalState = NULL;
		prob = -numeric_limits<float>::infinity();
	}
	else
	{
		if (finalState != NULL)
			*finalState = static_cast<const void*>(nGram);
		prob = nGram->GetScore();
	}
	return FloorScore(prob);
}
float LanguageModelInternal::GetValue(const Factor *factor0, const Factor *factor1, State* finalState) const
{
	float score;
	const NGramNode *nGram[2];

	nGram[1]		= GetLmID(factor1);
	if (nGram[1] == NULL)
	{
		if (finalState != NULL)
			*finalState = NULL;
		score = -numeric_limits<float>::infinity();
	}
	else
	{
		nGram[0] = nGram[1]->GetNGram(factor0);
		if (nGram[0] == NULL)
		{ // something unigram
			if (finalState != NULL)
				*finalState = static_cast<const void*>(nGram[1]);
			
			nGram[0]	= GetLmID(factor0);
			if (nGram[0] == NULL)
			{ // stops at unigram
				score = nGram[1]->GetScore();
			}
			else
			{	// unigram unigram
				score = nGram[1]->GetScore() + nGram[0]->GetLogBackOff();
			}
		}
		else
		{ // bigram
			if (finalState != NULL)
				*finalState = static_cast<const void*>(nGram[0]);
			score			= nGram[0]->GetScore();
		}
	}

	return FloorScore(score);

}

float LanguageModelInternal::GetValue(const Factor *factor0, const Factor *factor1, const Factor *factor2, State* finalState) const
{
	float score;
	const NGramNode *nGram[3];

	nGram[2]		= GetLmID(factor2);
	if (nGram[2] == NULL)
	{
		if (finalState != NULL)
			*finalState = NULL;
		score = -numeric_limits<float>::infinity();
	}
	else
	{
		nGram[1] = nGram[2]->GetNGram(factor1);
		if (nGram[1] == NULL)
		{ // something unigram
			if (finalState != NULL)
				*finalState = static_cast<const void*>(nGram[2]);
			
			nGram[1]	= GetLmID(factor1);
			if (nGram[1] == NULL)
			{ // stops at unigram
				score = nGram[2]->GetScore();
			}
			else
			{
				nGram[0] = nGram[1]->GetNGram(factor0);
				if (nGram[0] == NULL)
				{ // unigram unigram
					score = nGram[2]->GetScore() + nGram[1]->GetLogBackOff();
				}
				else
				{ // unigram bigram
					score = nGram[2]->GetScore() + nGram[1]->GetLogBackOff() + nGram[0]->GetLogBackOff();
				}	
			}			
		}
		else
		{ // trigram, or something bigram
			nGram[0] = nGram[1]->GetNGram(factor0);
			if (nGram[0] != NULL)
			{ // trigram
				if (finalState != NULL)
					*finalState = static_cast<const void*>(nGram[0]);
				score = nGram[0]->GetScore();
			}
			else
			{
				if (finalState != NULL)
					*finalState = static_cast<const void*>(nGram[1]);
				
				score			= nGram[1]->GetScore();
				nGram[1]	= nGram[1]->GetRootNGram();
				nGram[0]	= nGram[1]->GetNGram(factor0);
				if (nGram[0] == NULL)
				{ // just bigram
					// do nothing
				}
				else
				{
					score	+= nGram[0]->GetLogBackOff();
				}

			}
			// else do nothing. just use 1st bigram
		}
	}

	return FloorScore(score);

}

}

