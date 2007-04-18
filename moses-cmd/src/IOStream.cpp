// $Id$

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
#include "LatticePathList.h"
#include "StaticData.h"
#include "DummyScoreProducers.h"
#include "InputFileStream.h"

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
,m_nBestStream(NULL)
{
	m_surpressSingleBestOutput = false;
	if (nBestSize > 0)
	{
		if (nBestFilePath == "-")
		{
			m_nBestStream = &std::cout;
			m_surpressSingleBestOutput = true;
		} 
		else 
		{
			std::ofstream *nBestFile = new std::ofstream;
			m_nBestStream = nBestFile;
			nBestFile->open(nBestFilePath.c_str());
		}
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
,m_nBestStream(NULL)
{
	m_surpressSingleBestOutput = false;
	m_inputStream = m_inputFile;

	if (nBestSize > 0)
	{
		if (nBestFilePath == "-")
		{
			m_nBestStream = &std::cout;
			m_surpressSingleBestOutput = true;
		} 
		else 
		{
			std::ofstream *nBestFile = new std::ofstream;
			m_nBestStream = nBestFile;
			nBestFile->open(nBestFilePath.c_str());
		}
	}
}

IOStream::~IOStream()
{
	if (m_inputFile != NULL)
		delete m_inputFile;
	if (m_nBestStream != NULL && !m_surpressSingleBestOutput)
	{ // outputting n-best to file, rather than stdout. need to close file and delete obj
		delete m_nBestStream;
	}
}

InputType*IOStream::GetInput(InputType* inputType)
{
	if(inputType->Read(*m_inputStream, m_inputFactorOrder)) 
	{
		if (long x = inputType->GetTranslationId()) { if (x>=m_translationId) m_translationId = x+1; }
		else inputType->SetTranslationId(m_translationId++);
		  
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
void OutputSurface(std::ostream &out, const Phrase &phrase, const std::vector<FactorType> &outputFactorOrder, bool reportAllFactors)
{
	assert(outputFactorOrder.size() > 0);
	if (reportAllFactors == true) 
	{
		out << phrase;
	} 
	else 
	{
		size_t size = phrase.GetSize();
		for (size_t pos = 0 ; pos < size ; pos++)
		{
			const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[0]);
			out << *factor;
			
			for (size_t i = 1 ; i < outputFactorOrder.size() ; i++)
			{
				const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[i]);
				out << "|" << *factor;
			}
			out << " ";
		}
	}
}

void OutputSurface(std::ostream &out, const Hypothesis *hypo, const std::vector<FactorType> &outputFactorOrder
									 ,bool reportSegmentation, bool reportAllFactors)
{
	if ( hypo != NULL)
	{
		OutputSurface(out, hypo->GetPrevHypo(), outputFactorOrder, reportSegmentation, reportAllFactors);
		OutputSurface(out, hypo->GetCurrTargetPhrase(), outputFactorOrder, reportAllFactors);

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

void IOStream::OutputBestHypo(const Hypothesis *hypo, long /*translationId*/, bool reportSegmentation, bool reportAllFactors)
{
	if (hypo != NULL)
	{
		VERBOSE(1,"BEST TRANSLATION: " << *hypo << endl);
		VERBOSE(3,"Best path: ");
		Backtrack(hypo);
		VERBOSE(3,"0" << std::endl);

		if (!m_surpressSingleBestOutput)
		{
			OutputSurface(cout, hypo, m_outputFactorOrder, reportSegmentation, reportAllFactors);
			cout << endl;
		}
	}
	else
	{
		VERBOSE(1, "NO BEST TRANSLATION" << endl);
		if (!m_surpressSingleBestOutput)
		{
			cout << endl;
		}
	}
}

void IOStream::OutputNBestList(const LatticePathList &nBestList, long translationId)
{
	bool labeledOutput = StaticData::Instance().IsLabeledNBestList();
	bool includeAlignment = StaticData::Instance().NBestIncludesAlignment();
	
	LatticePathList::const_iterator iter;
	for (iter = nBestList.begin() ; iter != nBestList.end() ; ++iter)
	{
		const LatticePath &path = **iter;
		const std::vector<const Hypothesis *> &edges = path.GetEdges();

		// print the surface factor of the translation
		*m_nBestStream << translationId << " ||| ";
		for (int currEdge = (int)edges.size() - 1 ; currEdge >= 0 ; currEdge--)
		{
			const Hypothesis &edge = *edges[currEdge];
			OutputSurface(*m_nBestStream, edge.GetCurrTargetPhrase(), m_outputFactorOrder, false); // false for not reporting all factors
		}
		*m_nBestStream << " ||| ";

		// print the scores in a hardwired order
    // before each model type, the corresponding command-line-like name must be emitted
    // MERT script relies on this

		// basic distortion
		if (labeledOutput)
	    *m_nBestStream << "d: ";
		*m_nBestStream << path.GetScoreBreakdown().GetScoreForProducer(StaticData::Instance().GetDistortionScoreProducer()) << " ";

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
				  		*m_nBestStream << scores[j] << " ";
					}
				}
		}
			
		// lm
		const LMList& lml = StaticData::Instance().GetAllLM();
    if (lml.size() > 0) {
			if (labeledOutput)
	      *m_nBestStream << "lm: ";
		  LMList::const_iterator lmi = lml.begin();
		  for (; lmi != lml.end(); ++lmi) {
			  *m_nBestStream << path.GetScoreBreakdown().GetScoreForProducer(*lmi) << " ";
		  }
    }

		// translation components
		if (StaticData::Instance().GetInputType()==SentenceInput){  
			// translation components	for text input
			vector<PhraseDictionary*> pds = StaticData::Instance().GetPhraseDictionaries();
			if (pds.size() > 0) {
				if (labeledOutput)
					*m_nBestStream << "tm: ";
				vector<PhraseDictionary*>::iterator iter;
				for (iter = pds.begin(); iter != pds.end(); ++iter) {
					vector<float> scores = path.GetScoreBreakdown().GetScoresForProducer(*iter);
					for (size_t j = 0; j<scores.size(); ++j) 
						*m_nBestStream << scores[j] << " ";
				}
			}
		}
		else{		
			// translation components for Confusion Network input
			// first translation component has GetNumInputScores() scores from the input Confusion Network
			// at the beginning of the vector
			vector<PhraseDictionary*> pds = StaticData::Instance().GetPhraseDictionaries();
			if (pds.size() > 0) {
				vector<PhraseDictionary*>::iterator iter;
				
				iter = pds.begin();
				vector<float> scores = path.GetScoreBreakdown().GetScoresForProducer(*iter);
					
				size_t pd_numinputscore = (*iter)->GetNumInputScores();

				if (pd_numinputscore){
					
					if (labeledOutput)
						*m_nBestStream << "I: ";

					for (size_t j = 0; j < pd_numinputscore; ++j)
						*m_nBestStream << scores[j] << " ";
				}
					
					
				for (iter = pds.begin() ; iter != pds.end(); ++iter) {
					vector<float> scores = path.GetScoreBreakdown().GetScoresForProducer(*iter);
					
					size_t pd_numinputscore = (*iter)->GetNumInputScores();

					if (iter == pds.begin() && labeledOutput)
						*m_nBestStream << "tm: ";
					for (size_t j = pd_numinputscore; j < scores.size() ; ++j)
						*m_nBestStream << scores[j] << " ";
				}
			}
		}
		
		
		
		// word penalty
		if (labeledOutput)
	    *m_nBestStream << "w: ";
		*m_nBestStream << path.GetScoreBreakdown().GetScoreForProducer(StaticData::Instance().GetWordPenaltyProducer()) << " ";
		
		// generation
		vector<GenerationDictionary*> gds = StaticData::Instance().GetGenerationDictionaries();
    if (gds.size() > 0) {
			if (labeledOutput)
	      *m_nBestStream << "g: ";
		  vector<GenerationDictionary*>::iterator iter;
		  for (iter = gds.begin(); iter != gds.end(); ++iter) {
			  vector<float> scores = path.GetScoreBreakdown().GetScoresForProducer(*iter);
			  for (size_t j = 0; j<scores.size(); j++) {
				  *m_nBestStream << scores[j] << " ";
			  }
		  }
    }
		
		// total						
    *m_nBestStream << "||| " << path.GetTotalScore();
    if (includeAlignment) {
		*m_nBestStream << " |||";
		for (int currEdge = (int)edges.size() - 2 ; currEdge >= 0 ; currEdge--)
		{
			const Hypothesis &edge = *edges[currEdge];
			WordsRange sourceRange = edge.GetCurrSourceWordsRange();
			WordsRange targetRange = edge.GetCurrTargetWordsRange();
			*m_nBestStream << " " << sourceRange.GetStartPos();
			if (sourceRange.GetStartPos() < sourceRange.GetEndPos()) {
			  *m_nBestStream << "-" << sourceRange.GetEndPos();
			}
			*m_nBestStream << "=" << targetRange.GetStartPos();
			if (targetRange.GetStartPos() < targetRange.GetEndPos()) {
			  *m_nBestStream << "-" << targetRange.GetEndPos();
			}
		}
    }
    *m_nBestStream << endl;
	}

	*m_nBestStream<<std::flush;
}
