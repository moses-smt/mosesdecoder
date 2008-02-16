// $Id: DecodeStepTranslation.cpp 158 2007-10-22 00:47:01Z hieu $

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

#include "DecodeStepTranslation.h"
#include "DecodeStepGeneration.h"
#include "PhraseDictionaryMemory.h"
#include "TranslationOption.h"
#include "TranslationOptionCollection.h"
#include "PartialTranslOptColl.h"
#include "Util.h"
#include "PhraseDictionaryTreeAdaptor.h"
#include "DummyScoreProducers.h"
#include "FactorMask.h"
#include "DecodeStepGeneration.h"

size_t DecodeStepTranslation::s_id = INITIAL_DECODE_STEP_ID;

DecodeStepTranslation::DecodeStepTranslation(const WordPenaltyProducer &wpProducer
																						 , const DistortionScoreProducer &distortionScoreProducer
																						 , size_t maxNoTransOptPerCoverage)
: m_id(s_id++)
, m_wpProducer(wpProducer)
, m_distortionScoreProducer(distortionScoreProducer)
, m_maxNoTransOptPerCoverage(maxNoTransOptPerCoverage)
{
}

DecodeStepTranslation::~DecodeStepTranslation()
{
	RemoveAllInColl(m_genStepList);
}

/** returns phrase table (dictionary) for translation step */
const PhraseDictionary &DecodeStepTranslation::GetPhraseDictionary() const
{
  return *static_cast<const PhraseDictionary*>(m_ptr);
}

bool DecodeStepTranslation::Load(const std::string &origFilePath
													, size_t numScoreComponent
													, const std::vector<string> &inputFactorVector
													, const std::vector<FactorType> &input
													, const std::vector<FactorType> &output
													, const std::vector<float> &weight
													, size_t maxTargetPhrase
													, size_t numInputScores
													, const std::string &inputFileHash
													, const PrefixPhraseCollection &inputPrefix
													, ScoreIndexManager &scoreIndexManager)
{

	const StaticData &staticData = StaticData::Instance();
	string filePath = origFilePath;
	if (!FileExists(filePath+".binphr.idx"))
	{					
		// does cached filtering exist for this table, given input ?
		if (!FileExists(filePath) && FileExists(filePath + ".gz"))
			filePath += ".gz";
		string phraseTableHash	= GetMD5Hash(filePath);

		// input factors of input
		stringstream inputFactorsStrme("");
		for (size_t idx = 0 ; idx < inputFactorVector.size() ; ++idx)
			inputFactorsStrme << inputFactorVector[idx];


		string hashFilePath			= staticData.GetCachePath()
															+ PROJECT_NAME + "--1--" 	// filter file version
															+ inputFileHash + "--"
															+ inputFactorsStrme.str() // input factors of input
															+ phraseTableHash + "--"
															+ Join(",", input) // input factors of phrase table
															+ ".txt";
		bool filter;
		if (FileExists(hashFilePath))
		{ // load filtered file instead
			filter = false;
			filePath = hashFilePath;
		}
		else
		{ // load original file & create hash file
			filter = true;
		}

		// LOAD
		VERBOSE(2,"using standard phrase tables");
		PhraseDictionaryMemory *pd=new PhraseDictionaryMemory(numScoreComponent, scoreIndexManager);
		if (!pd->Load(input
						 , output
						 , filePath
						 , weight
						 , maxTargetPhrase
						 , staticData.GetAllLM()
						 , m_wpProducer.GetWPWeight()
						 , filter
						 , inputPrefix
						 , hashFilePath))
		{
			delete pd;
			return false;
		}
		m_outputFactorMask = pd->GetOutputFactorMask();
		m_nonConflictFactorMask = pd->GetOutputFactorMask();
		SetDictionary(pd);
	}
	else 
	{
		TRACE_ERR( "using binary phrase tables " << origFilePath << endl);
		PhraseDictionaryTreeAdaptor *pd=new PhraseDictionaryTreeAdaptor(
																							numScoreComponent
																							,(unsigned) numInputScores
																							, scoreIndexManager);
		if (!pd->Load(input,output,filePath,weight,
							 maxTargetPhrase,
							 staticData.GetAllLM(),
							 m_wpProducer.GetWPWeight()))
		{
			delete pd;
			return false;
		}
		m_outputFactorMask = pd->GetOutputFactorMask();
		m_nonConflictFactorMask = pd->GetOutputFactorMask();
		SetDictionary(pd);
	}
	
	return true;
}

TranslationOption *DecodeStepTranslation::MergeTranslation(const TranslationOption& oldTO, const TargetPhrase &targetPhrase) const
{
	/*
  if (IsFilteringStep()) {
    if (!oldTO.IsCompatible(targetPhrase, m_conflictFactors)) return 0;
  }

  TranslationOption *newTransOpt = new TranslationOption(oldTO);
  newTransOpt->MergeNewFeatures(targetPhrase, targetPhrase.GetScoreBreakdown(), m_newOutputFactors);
  return newTransOpt;
	*/
	return NULL;
}

void DecodeStepTranslation::Process(const InputType &source
															, PartialTranslOptColl &outputPartialTranslOptColl
															, size_t startPos
															, size_t endPos
															, bool adhereTableLimit) const
{
	const PhraseDictionary &phraseDictionary = GetPhraseDictionary();
	const size_t tableLimit = phraseDictionary.GetTableLimit();
	const WordsRange wordsRange(GetId(), startPos, endPos);
	const TargetPhraseCollection *phraseColl =	phraseDictionary.GetTargetPhraseCollection(source,wordsRange); 
	
	if (phraseColl != NULL)
	{
		VERBOSE(3,"[" << source.GetSubString(wordsRange) << "; " << startPos << "-" << endPos << "]\n");
			
		TargetPhraseCollection::const_iterator iterTargetPhrase, iterEnd;
		iterEnd = (!adhereTableLimit || tableLimit == 0 || phraseColl->GetSize() < tableLimit) ? phraseColl->end() : phraseColl->begin() + tableLimit;
		
		for (iterTargetPhrase = phraseColl->begin() ; iterTargetPhrase != iterEnd ; ++iterTargetPhrase)
		{
			const TargetPhrase	&targetPhrase = **iterTargetPhrase;
			TranslationOption *tranOpt = new TranslationOption(wordsRange, targetPhrase, m_id);
			if (m_genStepList.empty())
			{ // no generation to do
				outputPartialTranslOptColl.Add(tranOpt);
			}
			else
			{ // do generation
				ProcessGenerationStep(*tranOpt, outputPartialTranslOptColl);
				delete tranOpt;
			}
			
			VERBOSE(3,"\t" << targetPhrase << "\n");
		}
		VERBOSE(3,endl);
	}
}

void DecodeStepTranslation::ProcessGenerationStep(const TranslationOption &transOpt
																									, PartialTranslOptColl &outputPartialTranslOptColl) const
{
	PartialTranslOptColl *oldPtoc = new PartialTranslOptColl();

	// 1st gen step
	const DecodeStepGeneration *genStep = m_genStepList.front();
	genStep->Process(transOpt, *oldPtoc, true);

	// other gen steps, if any
	GenerationStepList::const_iterator iterGenStepList;
	for (iterGenStepList = ++m_genStepList.begin() ; iterGenStepList != m_genStepList.end() ; ++iterGenStepList)
	{
		genStep = *iterGenStepList;
		PartialTranslOptColl *newPtoc = new PartialTranslOptColl();

		// go thru each trans opt from old list, add to new list
		PartialTranslOptColl::iterator iterTransOptColl;
		for (iterTransOptColl = oldPtoc->begin() ; iterTransOptColl != oldPtoc->end() ; ++iterTransOptColl)
		{
			TranslationOption &transOpt = **iterTransOptColl;
			genStep->Process(transOpt, *newPtoc, true);
		}

		// get rid of old list
		RemoveAllInColl(*oldPtoc);
		delete oldPtoc;

		oldPtoc = newPtoc;
	}
	outputPartialTranslOptColl.Add(*oldPtoc);
	delete oldPtoc;
}

void DecodeStepTranslation::AddGenerationStep(const DecodeStepGeneration *genStep)
{
	m_genStepList.push_back(genStep);
	m_outputFactorMask.Merge(genStep->GetOutputFactorMask());
}

void DecodeStepTranslation::AddConflictMask(const FactorMask &conflict)
{
	m_conflictFactorMask.Merge(conflict);
}

