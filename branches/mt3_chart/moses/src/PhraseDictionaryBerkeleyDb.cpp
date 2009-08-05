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
#include "TargetPhraseCollection.h"
#include "DotChartBerkeleyDb.h"

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
		
	LoadTargetLookup();
	
	m_dbWrapper.Load(filePath);

	assert(m_dbWrapper.GetMisc("Version") == 1);
	assert(m_dbWrapper.GetMisc("NumSourceFactors") == input.size());
	assert(m_dbWrapper.GetMisc("NumTargetFactors") == output.size());
	assert(m_dbWrapper.GetMisc("NumScores") == weight.size());

	return true;
}

// PhraseDictionary impl
// for mert
void PhraseDictionaryBerkeleyDb::SetWeightTransModel(const std::vector<float> &weightT)
{
	assert(false);
}

//! find list of translations that can translates src. Only for phrase input
const TargetPhraseCollection *PhraseDictionaryBerkeleyDb::GetTargetPhraseCollection(const Phrase& src) const
{
	assert(false);
	return NULL;

	/*
	const StaticData &staticData = StaticData::Instance();
	
	TargetPhraseCollection *ret = new TargetPhraseCollection();
	m_cache.push_back(ret);
	Phrase *cachedSource = new Phrase(src);
	m_sourcePhrase.push_back(cachedSource);
	
	const MosesBerkeleyPt::SourcePhraseNode *nodeOld = new MosesBerkeleyPt::SourcePhraseNode(m_dbWrapper.GetInitNode());
	
	// find target phrases from tree
	size_t size = src.GetSize();
	for (size_t pos = 0; pos < size; ++pos)
	{
		// create on disk word from moses word
		const Word &origWord = src.GetWord(pos);

		const MosesBerkeleyPt::SourcePhraseNode *nodeNew;

		MosesBerkeleyPt::Word *searchWord = m_dbWrapper.ConvertFromMosesSource(m_inputFactorsVec, origWord);
		if (searchWord == NULL)
		{ // a vocab wasn't in there. definately can't find word in pt
			nodeNew = NULL;
		}
		else
		{	// search for word in node map
			nodeNew = m_dbWrapper.GetChild(*nodeOld, *searchWord);
		}
		
		delete nodeOld;
		nodeOld = nodeNew;
		
		if (nodeNew == NULL)
		{ // nothing found. end
			break;
		}
	} // for (size_t pos
	
	delete nodeOld;
	
	return ret;
	*/	
}
		
//! Create entry for translation of source to targetPhrase
void PhraseDictionaryBerkeleyDb::AddEquivPhrase(const Phrase &source, TargetPhrase *targetPhrase)
{
}

void PhraseDictionaryBerkeleyDb::InitializeForInput(const InputType& input)
{
	assert(m_runningNodesVec.size() == 0);
	size_t sourceSize = input.GetSize();
	m_runningNodesVec.resize(sourceSize);

	for (size_t ind = 0; ind < m_runningNodesVec.size(); ++ind)
	{
		ProcessedRuleBerkeleyDb *initProcessedRule = new ProcessedRuleBerkeleyDb(*m_initNode);

		ProcessedRuleStackBerkeleyDb *processedStack = new ProcessedRuleStackBerkeleyDb(sourceSize - ind + 1);
		processedStack->Add(0, initProcessedRule); // init rule. stores the top node in tree

		m_runningNodesVec[ind] = processedStack;
	}
}

void PhraseDictionaryBerkeleyDb::CleanUp()
{

}

void PhraseDictionaryBerkeleyDb::LoadTargetLookup()
{
	// TODO
}

	
}

