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

#include <fstream>
#include <string>
#include "GenerationDictionary.h"
#include "FactorCollection.h"
#include "Word.h"
#include "Util.h"
#include "InputFileStream.h"

using namespace std;

void GenerationDictionary::Load(const std::vector<FactorType> &input
																			, const std::vector<FactorType> &output
																			, FactorCollection &factorCollection
																			, const std::string &filePath
																			, float weight
																			, FactorDirection direction)
{	
	m_weight = weight;

	//factors	
	m_factorsUsed[Input] = new FactorTypeSet(input);
	m_factorsUsed[Output] = new FactorTypeSet(output);
	
	// data from file
	InputFileStream inFile(filePath);

	string line;
	while(getline(inFile, line)) 
	{
		vector<string> token = Tokenize( line );
		
		// add each line in generation file into class
		Word inputWord, outputWord;

		// create word with certain factors filled out

		// inputs
		vector<string> factorString = Tokenize( token[0], "|" );
		for (size_t i = 0 ; i < input.size() ; i++)
		{
			FactorType factorType = input[i];
			const Factor *factor = factorCollection.AddFactor( direction, factorType, factorString[i]);
			inputWord.SetFactor(factorType, factor);
		}

		factorString = Tokenize( token[1], "|" );
		for (size_t i = 0 ; i < output.size() ; i++)
		{
			FactorType factorType = output[i];
			
			const Factor *factor = factorCollection.AddFactor( direction, factorType, factorString[i]);
			outputWord.SetFactor(factorType, factor);
		}

		float score		= TransformScore(Scan<float>(token[2]));
		
		m_collection[inputWord][outputWord] = score;					
	}
	inFile.Close();

	// ??? temporary solution for unknown words
	// always assume it POS tags
	Word outputWord, word2, word3;
	const Factor *factor = factorCollection.AddFactor( Output, POS, "NNP");
	outputWord.SetFactor(POS, factor);
	m_unknownWord[outputWord] = 0.25f;

	factor = factorCollection.AddFactor( Output, POS, "NN");
	word2.SetFactor(POS, factor);
	m_unknownWord[word2] = 0.25f;

	factor = factorCollection.AddFactor( Output, POS, UNKNOWN_FACTOR);
	word3.SetFactor(POS, factor);
	m_unknownWord[word3] = 0.5f;
}

GenerationDictionary::~GenerationDictionary()
{
	for (size_t i = 0 ; i < m_factorsUsed.size() ; i++)
	{
		delete m_factorsUsed[i];
	}	
}

const OutputWordCollection *GenerationDictionary::FindWord(const FactorArray &factorArray) const
{
	const OutputWordCollection *ret;
	Word word;
	Word::Copy(word.GetFactorArray(), factorArray);
	
	std::map<Word , OutputWordCollection>::const_iterator iter = m_collection.find(word);
	if (iter == m_collection.end())
	{ // can't find source phrase
		ret = NULL;
	}
	else
	{
		ret = &iter->second;
	}
	return ret;
}

