// $Id: IOStream.cpp 1390 2007-05-16 13:16:00Z hieuhoang1972 $

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (c) 2006 University of Edinburgh
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, 
			this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, 
			this list of conditions and the following disclaimer in the documentation 
			and/or other materials provided with the distribution.
    * Neither the name of the University of Edinburgh nor the names of its contributors 
			may be used to endorse or promote products derived from this software 
			without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS 
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
***********************************************************************/

// example file on how to use moses library

#include <iostream>
#include "TypeDef.h"
#include "Util.h"
#include "IOStream.h"
#include "Hypothesis.h"
#include "WordsRange.h"
#include "TrellisPathList.h"
#include "StaticData.h"
#include "DummyScoreProducers.h"
#include "InputFileStream.h"
#include "DecodeStepGeneration.h"

using namespace std;

IOStream::IOStream(
				const vector<FactorType>				&inputFactorOrder
				, const vector<FactorType>			&outputFactorOrder
				, const FactorMask							&inputFactorUsed
				, size_t												nBestSize
				, const string									&nBestFilePath)
:m_inputFactorOrder(inputFactorOrder)
,m_outputFactorOrder(outputFactorOrder)
,m_inputFactorUsed(inputFactorUsed)
,m_inputFile(NULL)
,m_inputStream(&std::cin)
{
	if (nBestSize > 0)
	{
		m_nBestFile.open(nBestFilePath.c_str());
	}
}

IOStream::IOStream(const std::vector<FactorType>	&inputFactorOrder
						 , const std::vector<FactorType>	&outputFactorOrder
							, const FactorMask							&inputFactorUsed
							, size_t												nBestSize
							, const std::string							&nBestFilePath
							, const std::string							&inputFilePath)
:m_inputFactorOrder(inputFactorOrder)
,m_outputFactorOrder(outputFactorOrder)
,m_inputFactorUsed(inputFactorUsed)
,m_inputFilePath(inputFilePath)
,m_inputFile(new InputFileStream(inputFilePath))
{
	m_inputStream = m_inputFile;

	if (nBestSize > 0)
	{
		m_nBestFile.open(nBestFilePath.c_str());
	}
}

IOStream::~IOStream()
{
	if (m_inputFile != NULL)
		delete m_inputFile;
}

InputType*IOStream::GetInput(InputType* inputType)
{
	if(inputType->Read(*m_inputStream, m_inputFactorOrder)) 
	{
		inputType->SetTranslationId(m_translationId++);
		return inputType;
	}
	else 
	{
		delete inputType;
		return NULL;
	}
}

/***
 * print surface factor only for the given phrase
 */
void OutputSurface(std::ostream &out, const Phrase &phrase, const std::vector<FactorType> &outputFactorOrder)
{	
	assert(outputFactorOrder.size() > 0);
	size_t size = phrase.GetSize();
	for (size_t pos = 0 ; pos < size ; pos++)
	{
		const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[0]);
		if (factor == NULL)
			cerr << phrase << endl;
	
		out << *factor;
		
		for (size_t i = 1 ; i < outputFactorOrder.size() ; i++)
		{
			const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[i]);
			out << "|" << *factor;
		}
		out << " ";
	}
}

void OutputSurface(std::ostream &out, const Hypothesis *hypo, const std::vector<FactorType> &outputFactorOrder
									 ,bool reportSegmentation)
{
	if ( hypo != NULL)
	{
		OutputSurface(out, hypo->GetTargetPhrase(), outputFactorOrder);

		if (reportSegmentation == true
		    && hypo->GetCurrTargetPhrase().GetSize() > 0) {
			out << "|" << hypo->GetCurrSourceWordsRange().GetStartPos()
			    << "-" << hypo->GetCurrSourceWordsRange().GetEndPos() << "| ";
		}
	}
}

void IOStream::Backtrack(const Hypothesis *hypo){

	if (hypo->GetPrevHypo() != NULL) {
		VERBOSE(3,hypo->GetId() << " <= ");
		Backtrack(hypo->GetPrevHypo());
	}
}

void IOStream::OutputBestHypo(const Hypothesis *hypo, long /*translationId*/, bool reportSegmentation)
{	
	if (hypo != NULL)
	{
		TRACE_ERR("BEST TRANSLATION: " << *hypo << endl);
		VERBOSE(3,"Best path: ");
		Backtrack(hypo);
		VERBOSE(3,"0" << std::endl);

		OutputSurface(cout, hypo, m_outputFactorOrder, reportSegmentation);
	}
	else
	{
		TRACE_ERR("NO BEST TRANSLATION" << endl);
	}
	
	cout << endl;
}

#include "DecodeStepTranslation.h"

void IOStream::OutputNBestList(const TrellisPathList &nBestList, long translationId)
{
	m_nBestFile.setf(std::ios::fixed); 
	m_nBestFile.precision(5);

	bool labeledOutput = StaticData::Instance().IsLabeledNBestList();

	const DecodeStepCollection &translateStepList = StaticData::Instance().GetDecodeStepCollection();
	DecodeStepCollection::const_iterator iterTranslateStepList;

	//list<const DecodeStep*> &generateStepList = StaticData::Instance().GetDecodeStepCollection().GetDecodeStepList(Generate);

	TrellisPathList::const_iterator iter;
	for (iter = nBestList.begin() ; iter != nBestList.end() ; ++iter)
	{
		const TrellisPath &path = **iter;
		const std::vector<const Hypothesis *> &edges = path.GetEdges();

		// print the surface factor of the translation
		m_nBestFile << translationId << " ||| ";
		OutputSurface(m_nBestFile, path.GetTargetPhrase(), m_outputFactorOrder);
		m_nBestFile << " ||| ";

		// print the scores in a hardwired order
    // before each model type, the corresponding command-line-like name must be emitted
    // MERT script relies on this

		// basic distortion
		if (labeledOutput)
	    m_nBestFile << "d: ";

		for (iterTranslateStepList = translateStepList.begin(); iterTranslateStepList != translateStepList.end(); ++iterTranslateStepList) 
		{
			const DecodeStepTranslation &step = static_cast<const DecodeStepTranslation&>(**iterTranslateStepList);
			const DistortionScoreProducer &distortionScoreProducer = step.GetDistortionScoreProducer();
			m_nBestFile << path.GetScoreBreakdown().GetScoreForProducer(&distortionScoreProducer) << " ";
		}

//		reordering
		vector<LexicalReordering*> rms = StaticData::Instance().GetReorderModels();
		if(rms.size() > 0)
		{
				vector<LexicalReordering*>::iterator iter;
				for(iter = rms.begin(); iter != rms.end(); ++iter)
				{
					vector<float> scores = path.GetScoreBreakdown().GetScoresForProducer(*iter);
					for (size_t j = 0; j<scores.size(); ++j) 
					{
				  		m_nBestFile << scores[j] << " ";
					}
				}
		}
			
		// lm
		const LMList& lml = StaticData::Instance().GetAllLM();
    if (lml.size() > 0) {
			if (labeledOutput)
	      m_nBestFile << "lm: ";
		  LMList::const_iterator lmi = lml.begin();
		  for (; lmi != lml.end(); ++lmi) {
			  m_nBestFile << path.GetScoreBreakdown().GetScoreForProducer(*lmi) << " ";
		  }
    }

		// translation components
		if (StaticData::Instance().GetInputType() == SentenceInput){  
			// translation components	for text input
			if (translateStepList.GetSize() > 0) {
				if (labeledOutput)
					m_nBestFile << "tm: ";
				list<const DecodeStep*>::iterator iter;
				for (iterTranslateStepList = translateStepList.begin(); iterTranslateStepList != translateStepList.end(); ++iterTranslateStepList) 
				{
					vector<float> scores = path.GetScoreBreakdown().GetScoresForProducer(&(*iterTranslateStepList)->GetPhraseDictionary());
					for (size_t j = 0; j<scores.size(); ++j) 
						m_nBestFile << scores[j] << " ";
				}
			}
		}
		else{		
			// translation components for Confusion Network/lattice input
			// first translation component has GetNumInputScores() scores from the input Confusion Network
			// at the beginning of the vector
			if (translateStepList.GetSize() > 0) {
				
				iterTranslateStepList = translateStepList.begin();
				const PhraseDictionary &phraseDict = (*iterTranslateStepList)->GetPhraseDictionary();
				vector<float> scores = path.GetScoreBreakdown().GetScoresForProducer(&phraseDict);
					
				size_t pd_numinputscore = phraseDict.GetNumInputScores();

				if (pd_numinputscore){
					
					if (labeledOutput)
						m_nBestFile << "I: ";

					for (size_t j = 0; j < pd_numinputscore; ++j)
						m_nBestFile << scores[j] << " ";
				}
					
					
				for (iterTranslateStepList = translateStepList.begin() ; iterTranslateStepList != translateStepList.end(); ++iterTranslateStepList) {
					vector<float> scores = path.GetScoreBreakdown().GetScoresForProducer(&phraseDict);
					
					size_t pd_numinputscore = phraseDict.GetNumInputScores();

					if (iterTranslateStepList == translateStepList.begin() && labeledOutput)
						m_nBestFile << "tm: ";
					for (size_t j = pd_numinputscore; j < scores.size() ; ++j)
						m_nBestFile << scores[j] << " ";
				}
			}
		}
		
		
		
		// word penalty
		if (labeledOutput)
	    m_nBestFile << "w: ";
		for (iterTranslateStepList = translateStepList.begin() ; iterTranslateStepList != translateStepList.end(); ++iterTranslateStepList) 
		{
			const DecodeStepTranslation &step = **iterTranslateStepList;
			m_nBestFile << path.GetScoreBreakdown().GetScoreForProducer(&step.GetWordPenaltyProducer()) << " ";
		}
		
		// generation
		if (labeledOutput)
      m_nBestFile << "g: ";
		for (iterTranslateStepList = translateStepList.begin() ; iterTranslateStepList != translateStepList.end(); ++iterTranslateStepList) 
		{
			const DecodeStepTranslation &tranStep = **iterTranslateStepList;
			const GenerationStepList &genStepList = tranStep .GetDecodeStepGenerationList();
			GenerationStepList::const_iterator iterGenStepList;

			for (iterGenStepList = genStepList.begin() ; iterGenStepList != genStepList.end(); ++iterGenStepList)
			{
				const DecodeStepGeneration *genStep = *iterGenStepList;
				const GenerationDictionary &genDict = genStep->GetGenerationDictionary();
				vector<float> scores = path.GetScoreBreakdown().GetScoresForProducer(&genDict);
				for (size_t j = 0; j<scores.size(); j++) 
				{
					m_nBestFile << scores[j] << " ";
				}
			}
	  }

		// total						
		m_nBestFile << "||| " << path.GetTotalScore() << endl;
	}

	m_nBestFile<<std::flush;
}
