
#include "LanguageModelInternal.h"
#include "FactorCollection.h"
#include "NGramNode.h"
#include "InputFileStream.h"

using namespace std;

LanguageModelInternal::LanguageModelInternal(bool registerScore)
:LanguageModelSingleFactor(registerScore)
{
}

void LanguageModelInternal::Load(const std::string &filePath
					, FactorCollection &factorCollection
					, FactorType factorType
					, float weight
					, size_t nGramOrder)
{
	assert(nGramOrder <= 3);

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
	TRACE_ERR(filePath << endl);

	InputFileStream 	inFile(filePath);

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

}

float LanguageModelInternal::GetValue(const std::vector<const Word*> &contextFactor
												, State* finalState
												, unsigned int* len) const
{

	return 0;
}

