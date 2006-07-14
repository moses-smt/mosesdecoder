// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include <assert.h>
#include <limits>
#include <iostream>
#include <fstream>

#include "NGramNode.h"

#include "LanguageModel.h"
#include "TypeDef.h"
#include "Util.h"
#include "FactorCollection.h"
#include "Phrase.h"

using namespace std;

void LanguageModel::CalcScore(const Phrase &phrase
														, float &fullScore
														, float &ngramScore
														, list< std::pair<size_t, float> >	&ngramComponent) const
{
	fullScore	= 0;
	ngramScore	= 0;
	FactorType factorType = GetFactorType();

	size_t phraseSize = phrase.GetSize();
	vector<const Factor*> contextFactor;
	contextFactor.reserve(m_nGramOrder);
		
	// start of sentence
	for (size_t currPos = 0 ; currPos < m_nGramOrder - 1 && currPos < phraseSize ; currPos++)
	{
		contextFactor.push_back(phrase.GetFactor(currPos, factorType));		
		fullScore += GetValue(contextFactor);
	}
	
	if (phraseSize >= m_nGramOrder)
	{
		contextFactor.push_back(phrase.GetFactor(m_nGramOrder - 1, factorType));
		ngramScore = GetValue(contextFactor);
	}
	
	// main loop
	for (size_t currPos = m_nGramOrder; currPos < phraseSize ; currPos++)
	{ // used by hypo to speed up lm score calc
		for (size_t currNGramOrder = 0 ; currNGramOrder < m_nGramOrder - 1 ; currNGramOrder++)
		{
			contextFactor[currNGramOrder] = contextFactor[currNGramOrder + 1];
		}
		contextFactor[m_nGramOrder - 1] = phrase.GetFactor(currPos, factorType);
		
		ngramScore += GetValue(contextFactor);		
	}
	fullScore += ngramScore;
	
#ifdef N_BEST
				size_t lmId = GetId();
				pair<size_t, float> store(lmId, ngramScore);
				ngramComponent.push_back(store);
#endif
}

#ifdef LM_SRI

LanguageModel::LanguageModel()
:m_srilmVocab()
,m_srilmModel(m_srilmVocab, 3)
{
	m_srilmModel.skipOOVs() = false;
}

void LanguageModel::Load(size_t id
												, const std::string &fileName
												, FactorCollection &factorCollection
												, FactorType factorType
												, float weight
												, size_t nGramOrder)
{
	m_id					= id;
	m_factorType 	= factorType;
	m_weight			= weight;
	m_nGramOrder	= nGramOrder;

	File file( fileName.c_str(), "r" );
	if (m_srilmModel.read(file))
	{
	}
	else
	{
		TRACE_ERR("warning/failed loading language model" << endl);
	}
	// LM can be ok, just outputs warnings
	CreateFactors(factorCollection);		

}

void LanguageModel::CreateFactors(FactorCollection &factorCollection)
{ // add factors which have srilm id
	
	VocabString str;
	LmId lmId;
	VocabIter iter(m_srilmVocab);
	while ( (str = iter.next()) != NULL)
	{
		LmId lmId = GetLmID(str);
		factorCollection.AddFactor(Output, m_factorType, str, lmId);
	}
	
	lmId = GetLmID(SENTENCE_START);
	m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, SENTENCE_START, lmId);
	lmId = GetLmID(SENTENCE_END);
	m_sentenceEnd		= factorCollection.AddFactor(Output, m_factorType, SENTENCE_END, lmId);
	
}

float LanguageModel::GetValue(const vector<const Factor*> &contextFactor) const
{
	// set up context
	size_t count = contextFactor.size();
	LmId *context = (LmId*) malloc(count * sizeof(LmId));
	for (size_t i = 0 ; i < count - 1 ; i++)
	{
		context[i] = GetLmID(contextFactor[count-2-i]);
	}
	context[count-1] = Vocab_None;
	
	// call sri lm fn
	float ret = GetValue(GetLmID(contextFactor[count-1]), context);
	free(context);
	return ret;
}

#endif
#ifdef LM_INTERNAL

#include "InputFileStream.h"

// static variable init
const LmId LanguageModel::UNKNOWN_LM_ID = NULL;

// class methods
LanguageModel::LanguageModel()
{
}

void LanguageModel::Load(size_t id
											, const std::string &fileName
											, FactorCollection &factorCollection
											, FactorType factorType
											, float weight
											, size_t nGramOrder)
{
	m_id					= id;
	m_factorType	= factorType;
	m_weight			= weight;
	m_nGramOrder	= nGramOrder;
	// make sure start & end tags in factor collection
	m_sentenceStart	= factorCollection.AddFactor(Output, m_factorType, SENTENCE_START);
	m_sentenceEnd		= factorCollection.AddFactor(Output, m_factorType, SENTENCE_END);

	// read in file
	TRACE_ERR(fileName << endl);

	InputFileStream 	inFile(fileName);

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
				factorCollection.SetFactorLmId(factor, rootNGram);

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

float LanguageModel::GetValue(const std::vector<const Factor*> &contextFactor) const
{
	float score;
	size_t nGramOrder = contextFactor.size();
	assert(nGramOrder <= 3);
	
	if (nGramOrder == 1)
		score = GetValue(contextFactor[0]);
	else if (nGramOrder == 2)
		score = GetValue(contextFactor[0], contextFactor[1]);
	else if (nGramOrder == 3)
		score = GetValue(contextFactor[0], contextFactor[1], contextFactor[2]);

	return FloorSRIScore(score);
}

float LanguageModel::GetValue(const Factor *factor0) const
{
	float prob;
	const NGramNode *nGram		= factor0->GetLmId();
	if (nGram == NULL)
	{
		prob = -numeric_limits<float>::infinity();
	}
	else
	{
		prob = nGram->GetScore();
	}
	return FloorSRIScore(prob);
}
float LanguageModel::GetValue(const Factor *factor0, const Factor *factor1) const
{
	float score;
	const NGramNode *nGram[2];

	nGram[1]		= factor1->GetLmId();
	if (nGram[1] == NULL)
	{
		score = -numeric_limits<float>::infinity();
	}
	else
	{
		nGram[0] = nGram[1]->GetNGram(factor0);
		if (nGram[0] == NULL)
		{ // something unigram
			nGram[0]	= factor0->GetLmId();
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
			score			= nGram[0]->GetScore();
		}
	}

	return FloorSRIScore(score);

}

float LanguageModel::GetValue(const Factor *factor0, const Factor *factor1, const Factor *factor2) const
{
	float score;
	const NGramNode *nGram[3];

	nGram[2]		= factor2->GetLmId();
	if (nGram[2] == NULL)
	{
		score = -numeric_limits<float>::infinity();
	}
	else
	{
		nGram[1] = nGram[2]->GetNGram(factor1);
		if (nGram[1] == NULL)
		{ // something unigram
			nGram[1]	= factor1->GetLmId();
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
				score = nGram[0]->GetScore();
			}
			else
			{
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

	return FloorSRIScore(score);

}

#endif

