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
#include "Staticdata.h"

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
	
	LoadTargetLookup();
	
	m_dbWrapper.Open(filePath);
	
	return true;
}

// PhraseDictionary impl
// for mert
void PhraseDictionaryBerkeleyDb::SetWeightTransModel(const std::vector<float> &weightT)
{
	assert(false);
}

#include "TargetPhraseCollection.h"

//! find list of translations that can translates src. Only for phrase input
const TargetPhraseCollection *PhraseDictionaryBerkeleyDb::GetTargetPhraseCollection(const Phrase& src) const
{
	const StaticData &staticData = StaticData::Instance();
	
	TargetPhraseCollection *ret = new TargetPhraseCollection();
	m_cache.push_back(ret);
	Phrase *cachedSource = new Phrase(src);
	m_sourcePhrase.push_back(cachedSource);
	
	const MosesBerkeleyPt::SourcePhraseNode *nodeOld = new MosesBerkeleyPt::SourcePhraseNode(*m_initNode);
	
	// find target phrases from tree
	size_t size = src.GetSize();
	for (size_t pos = 0; pos < size; ++pos)
	{
		// create on disk word from moses word
		const Word &origWord = src.GetWord(pos);
		MosesBerkeleyPt::Word searchWord(m_inputFactorsVec.size());
		
		const MosesOnDiskPt::SourcePhraseNode *nodeNew;
		
		bool success = searchWord.ConvertFromMoses(m_inputFactorsVec, origWord, m_vocabLookup);
		if (!success)
		{
			nodeNew = NULL;
		}
		else
		{	// search for word in node map
			nodeNew = nodeOld->GetChild(searchWord, m_sourceFile, m_inputFactorsVec);
		}
		
		delete nodeOld;
		nodeOld = nodeNew;
		
		if (nodeNew == NULL)
		{ // nothing found. end
			break;
		}
	} // for (size_t pos
	

	
	return NULL;
}


//! Create entry for translation of source to targetPhrase
void PhraseDictionaryBerkeleyDb::AddEquivPhrase(const Phrase &source, TargetPhrase *targetPhrase)
{
}

void PhraseDictionaryBerkeleyDb::CleanUp()
{

}

void PhraseDictionaryBerkeleyDb::LoadTargetLookup()
{
	// TODO
}

	
}

