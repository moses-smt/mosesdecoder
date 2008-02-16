// $Id: DecodeStepTranslation.h 158 2007-10-22 00:47:01Z hieu $

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

#pragma once

#include <list>
#include "DecodeStep.h"
#include "PrefixPhraseCollection.h"

class PhraseDictionary;
class TargetPhrase;
class WordPenaltyProducer;
class DistortionScoreProducer;
class DecodeStepGeneration;

typedef std::list<const DecodeStepGeneration*> GenerationStepList;

class DecodeStepTranslation : public DecodeStep
{
protected:
	static size_t s_id;
	
	FactorMask m_outputFactorMask, m_conflictFactorMask, m_nonConflictFactorMask;
	GenerationStepList m_genStepList;
	size_t m_id, m_maxNoTransOptPerCoverage;
	const WordPenaltyProducer &m_wpProducer;
	const DistortionScoreProducer &m_distortionScoreProducer;
	/** create new TranslationOption from merging oldTO with mergePhrase
		This function runs IsCompatible() to ensure the two can be merged
	*/
	TranslationOption *MergeTranslation(const TranslationOption& oldTO, const TargetPhrase &targetPhrase) const;

public:
	explicit DecodeStepTranslation(const DecodeStepTranslation&); // not implemented
	DecodeStepTranslation(const WordPenaltyProducer &m_wpProducer
												, const DistortionScoreProducer &distortionScoreProducer
												, size_t maxNoTransOptPerCoverage);
	~DecodeStepTranslation();

	DecodeType GetDecodeType() const
	{ return Translate; }
	
	const FactorMask& GetOutputFactorMask() const
	{ return m_outputFactorMask; }
	const FactorMask& GetConflictFactorMask() const
	{ return m_conflictFactorMask; }
	const FactorMask& GetNonConflictFactorMask() const
	{ return m_nonConflictFactorMask; }
	
	size_t GetId() const 
	{ return m_id;}
	size_t GetMaxNoTransOptPerCoverage() const 
	{ return m_maxNoTransOptPerCoverage;	}

	const WordPenaltyProducer &GetWordPenaltyProducer() const
	{ return m_wpProducer; }
	const DistortionScoreProducer &GetDistortionScoreProducer() const
	{ return m_distortionScoreProducer; }

	/** returns phrase table (dictionary) for translation step */
	const PhraseDictionary &GetPhraseDictionary() const;

	//! returns list of gen step that came after this trans step
	const GenerationStepList &GetDecodeStepGenerationList() const
	{ return m_genStepList; }

	void AddConflictMask(const FactorMask &conflict);

	/** initialize list of partial translation options by applying the first translation step 
	* Ideally, this function should be in DecodeStepTranslation class
	*/
	void Process(const InputType &source
															, PartialTranslOptColl &outputPartialTranslOptColl
															, size_t startPos
															, size_t endPos
															, bool adhereTableLimit) const;
	bool Load(const std::string &origFilePath
													, size_t numScoreComponent
													, const std::vector<std::string> &inputFactorVector
													, const std::vector<FactorType> &input
													, const std::vector<FactorType> &output
													, const std::vector<float> &weight
													, size_t maxTargetPhrase
													, size_t numInputScores
													, const std::string &inputFileHash
													, const PrefixPhraseCollection &inputPrefix
													, ScoreIndexManager &scoreIndexManager);
	void AddGenerationStep(const DecodeStepGeneration *genStep);

	void ProcessGenerationStep(const TranslationOption &transOpt
													, PartialTranslOptColl &outputPartialTranslOptColl) const;

};

