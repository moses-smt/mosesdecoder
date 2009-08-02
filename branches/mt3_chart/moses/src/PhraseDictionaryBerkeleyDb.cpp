/*
 *  PhraseDictionaryBerkeleyDb.cpp
 *  moses
 *
 *  Created by Hieu Hoang on 31/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "PhraseDictionaryBerkeleyDb.h"
#include "InputFileStream.h"

using namespace std;

namespace Moses
{
PhraseDictionaryBerkeleyDb::~PhraseDictionaryBerkeleyDb()
{
}

bool PhraseDictionaryBerkeleyDb::Load(const std::vector<FactorType> &input
					, const std::vector<FactorType> &output
					, const std::string &filePath
					, const std::vector<float> &weight
					, size_t tableLimit)
{
	m_tableLimit = tableLimit;
	m_inputFactors = FactorMask(input);
	m_outputFactors = FactorMask(output);
	m_inputFactorsVec		= input;
	m_outputFactorsVec	= output;
	
	m_weight = weight;
	
	// load vocab
	InputFileStream vocabFile(filePath + "/vocab.db");
	string line;
	//int lineNo = 0;
	
	while( !getline(vocabFile, line, '\n').eof())
	{
		vector<string> vecStr = Tokenize(line);
		assert(vecStr.size() == 2);
		MosesBerkeleyPt::VocabId vocabId = Scan<MosesBerkeleyPt::VocabId>(vecStr[1]);
		m_vocabLookup[ vecStr[0] ] = vocabId;
	}
	

	return true;
}

// PhraseDictionary impl
// for mert
void PhraseDictionaryBerkeleyDb::SetWeightTransModel(const std::vector<float> &weightT)
{
}


//! find list of translations that can translates src. Only for phrase input
const TargetPhraseCollection *PhraseDictionaryBerkeleyDb::GetTargetPhraseCollection(const Phrase& src) const
{

	return NULL;
}


//! Create entry for translation of source to targetPhrase
void PhraseDictionaryBerkeleyDb::AddEquivPhrase(const Phrase &source, TargetPhrase *targetPhrase)
{
}

void PhraseDictionaryBerkeleyDb::CleanUp()
{

}

}

