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
#include "StaticData.h"

using namespace std;

GenerationDictionary::GenerationDictionary(size_t numFeatures)
  : Dictionary(numFeatures)
{
	const_cast<ScoreIndexManager&>(StaticData::Instance()->GetScoreIndexManager()).AddScoreProducer(this);
}

void GenerationDictionary::Load(const std::vector<FactorType> &input
																			, const std::vector<FactorType> &output
																			, FactorCollection &factorCollection
																			, const std::string &filePath
																			, FactorDirection direction
																			, bool forceSingleFeatureValue)
{	
	const size_t numFeatureValuesInConfig = this->GetNumScoreComponents();

  // old hack - originally, moses assumed single generation values
	if (forceSingleFeatureValue) {
		assert(numFeatureValuesInConfig == 1);
	}

	//factors	
	m_inputFactors = FactorMask(input);
	m_outputFactors = FactorMask(output);
	VERBOSE(2,"GenerationDictionary: input=" << m_inputFactors << "  output=" << m_outputFactors << std::endl);
	
	// data from file
	InputFileStream inFile(filePath);
	if (!inFile.good()) {
		std::cerr << "Couldn't read " << filePath << std::endl;
		exit(1);
	}

	m_filename = filePath;
	string line;
	size_t lineNum = 0;
	while(getline(inFile, line)) 
	{
		++lineNum;
		vector<string> token = Tokenize( line );
		
		// add each line in generation file into class
		Word *inputWord = new Word();  // deleted in destructor
		Word outputWord;

		// create word with certain factors filled out

		// inputs
		vector<string> factorString = Tokenize( token[0], "|" );
		for (size_t i = 0 ; i < input.size() ; i++)
		{
			FactorType factorType = input[i];
			const Factor *factor = factorCollection.AddFactor( direction, factorType, factorString[i]);
			inputWord->SetFactor(factorType, factor);
		}

		factorString = Tokenize( token[1], "|" );
		for (size_t i = 0 ; i < output.size() ; i++)
		{
			FactorType factorType = output[i];
			
			const Factor *factor = factorCollection.AddFactor( direction, factorType, factorString[i]);
			outputWord.SetFactor(factorType, factor);
		}

		size_t numFeaturesInFile = token.size() - 2;
		if (forceSingleFeatureValue) numFeaturesInFile = 1;
		if (numFeaturesInFile < numFeatureValuesInConfig) {
			std::cerr << filePath << ":" << lineNum << ": expected " << numFeatureValuesInConfig
								<< " feature values, but found " << numFeaturesInFile << std::endl;
			exit(1);
		}
		std::vector<float> scores(numFeatureValuesInConfig, 0.0f);
		for (size_t i = 0; i < numFeatureValuesInConfig; i++)
			scores[i] = TransformScore(Scan<float>(token[2+i]));
		
		m_collection[inputWord][outputWord].Assign(this, scores);
//		std::cerr << "CD: " << m_collection[inputWord][outputWord] << std::endl;
	}
	inFile.Close();
}

GenerationDictionary::~GenerationDictionary()
{
	std::map<const Word* , OutputWordCollection, WordComparer>::const_iterator iter;
	for (iter = m_collection.begin() ; iter != m_collection.end() ; ++iter)
	{
		delete iter->first;
	}
}

size_t GenerationDictionary::GetNumScoreComponents() const
{
  return this->GetNoScoreComponents();
}

const std::string GenerationDictionary::GetScoreProducerDescription() const
{
  return "Generation score, file=" + m_filename;
}

const OutputWordCollection *GenerationDictionary::FindWord(const Word &word) const
{
	const OutputWordCollection *ret;
	
	std::map<const Word* , OutputWordCollection, WordComparer>::const_iterator 
				iter = m_collection.find(&word);
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

