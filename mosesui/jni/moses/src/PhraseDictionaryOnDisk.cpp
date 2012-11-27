// $Id: PhraseDictionaryOnDisk.cpp 3616 2010-10-12 14:10:19Z hieuhoang1972 $
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang
 
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

#include "PhraseDictionaryOnDisk.h"
#include "InputFileStream.h"
#include "StaticData.h"
#include "TargetPhraseCollection.h"
#include "DotChartOnDisk.h"

using namespace std;


namespace Moses
{
PhraseDictionaryOnDisk::~PhraseDictionaryOnDisk()
{
	CleanUp();
}

bool PhraseDictionaryOnDisk::Load(const std::vector<FactorType> &input
					, const std::vector<FactorType> &output
					, const std::string &filePath
					, const std::vector<float> &weight
					, size_t tableLimit
                    , const LMList& languageModels
                    , const WordPenaltyProducer* wpProducer)
{
  m_languageModels = &(languageModels);
  m_wpProducer = wpProducer;
  m_filePath = filePath;
	m_tableLimit = tableLimit;
	m_inputFactorsVec		= input;
	m_outputFactorsVec	= output;
	
	m_weight = weight;
		
	LoadTargetLookup();
	
	if (!m_dbWrapper.BeginLoad(filePath))
		return false;

	assert(m_dbWrapper.GetMisc("Version") == 3);
	assert(m_dbWrapper.GetMisc("NumSourceFactors") == input.size());
	assert(m_dbWrapper.GetMisc("NumTargetFactors") == output.size());
	assert(m_dbWrapper.GetMisc("NumScores") == weight.size());

	return true;
}

//! find list of translations that can translates src. Only for phrase input
const TargetPhraseCollection *PhraseDictionaryOnDisk::GetTargetPhraseCollection(const Phrase& src) const
{
	assert(false);
	return NULL;
}
	
void PhraseDictionaryOnDisk::AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase)
{
	assert(false); // TODO
}
	
		
//! Create entry for translation of source to targetPhrase
void PhraseDictionaryOnDisk::AddEquivPhrase(const Phrase &source, TargetPhrase *targetPhrase)
{
}

void PhraseDictionaryOnDisk::InitializeForInput(const InputType& input)
{
	assert(m_runningNodesVec.size() == 0);
	size_t sourceSize = input.GetSize();
	m_runningNodesVec.resize(sourceSize);

	for (size_t ind = 0; ind < m_runningNodesVec.size(); ++ind)
	{
		ProcessedRuleOnDisk *initProcessedRule = new ProcessedRuleOnDisk(m_dbWrapper.GetRootSourceNode());

		ProcessedRuleStackOnDisk *processedStack = new ProcessedRuleStackOnDisk(sourceSize - ind + 1);
		processedStack->Add(0, initProcessedRule); // init rule. stores the top node in tree

		m_runningNodesVec[ind] = processedStack;
	}

}

void PhraseDictionaryOnDisk::CleanUp()
{
	std::map<UINT64, const TargetPhraseCollection*>::const_iterator iterCache;
	for (iterCache = m_cache.begin(); iterCache != m_cache.end(); ++iterCache)
	{
		delete iterCache->second;
	}
	m_cache.clear();
	
	RemoveAllInColl(m_runningNodesVec);
	RemoveAllInColl(m_sourcePhraseNode);	
}

void PhraseDictionaryOnDisk::LoadTargetLookup()
{
	// TODO
}

	
}

