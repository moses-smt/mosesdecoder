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
#include "IOWrapper.h"
#include "WordsRange.h"
#include "StaticData.h"
#include "DummyScoreProducers.h"
#include "InputFileStream.h"
#include "PhraseDictionary.h"
#include "../../moses-chart/src/ChartTrellisPathList.h"
#include "../../moses-chart/src/ChartTrellisPath.h"

using namespace std;
using namespace Moses;
using namespace MosesChart;

IOWrapper::IOWrapper(const std::vector<FactorType>	&inputFactorOrder
						 , const std::vector<FactorType>	&outputFactorOrder
							, const FactorMask							&inputFactorUsed
							, size_t												nBestSize
							, const std::string							&nBestFilePath
							, const std::string							&inputFilePath)
:m_inputFactorOrder(inputFactorOrder)
,m_outputFactorOrder(outputFactorOrder)
,m_inputFactorUsed(inputFactorUsed)
,m_nBestStream(NULL)
,m_outputSearchGraphStream(NULL)
,m_detailedTranslationReportingStream(0)
,m_inputFilePath(inputFilePath)
{
	const StaticData &staticData = StaticData::Instance();

  if (m_inputFilePath.empty())
  {
    m_inputStream = &std::cin;
  }
  else
  {
    m_inputStream = new InputFileStream(inputFilePath);
  }

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

	// search graph output
	if (staticData.GetOutputSearchGraph())
	{
	  string fileName = staticData.GetParam("output-search-graph")[0];
	  std::ofstream *file = new std::ofstream;
	  m_outputSearchGraphStream = file;
	  file->open(fileName.c_str());
	}

  // detailed translation reporting
  if (staticData.IsDetailedTranslationReportingEnabled())
  {
    const std::string &path = staticData.GetDetailedTranslationReportingFilePath();
    m_detailedTranslationReportingStream = new std::ofstream(path.c_str());
  }
}

IOWrapper::~IOWrapper()
{
  if (!m_inputFilePath.empty())
  {
    delete m_inputStream;
  }
	if (m_nBestStream != NULL && !m_surpressSingleBestOutput)
	{ // outputting n-best to file, rather than stdout. need to close file and delete obj
		delete m_nBestStream;
	}

	if (m_outputSearchGraphStream != NULL)
	{
	  delete m_outputSearchGraphStream;
	}

  delete m_detailedTranslationReportingStream;
}

InputType*IOWrapper::GetInput(InputType* inputType)
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

void OutputSurface(std::ostream &out, const MosesChart::Hypothesis *hypo, const std::vector<FactorType> &outputFactorOrder
									 ,bool reportSegmentation, bool reportAllFactors)
{
	if ( hypo != NULL)
	{
		//OutputSurface(out, hypo->GetCurrTargetPhrase(), outputFactorOrder, reportAllFactors);

		const vector<const MosesChart::Hypothesis*> &prevHypos = hypo->GetPrevHypos();

		vector<const MosesChart::Hypothesis*>::const_iterator iter;
		for (iter = prevHypos.begin(); iter != prevHypos.end(); ++iter)
		{
			const MosesChart::Hypothesis *prevHypo = *iter;

			OutputSurface(out, prevHypo, outputFactorOrder, reportSegmentation, reportAllFactors);
		}
	}
}

void IOWrapper::Backtrack(const MosesChart::Hypothesis *hypo)
{
	const vector<const MosesChart::Hypothesis*> &prevHypos = hypo->GetPrevHypos();

	vector<const MosesChart::Hypothesis*>::const_iterator iter;
	for (iter = prevHypos.begin(); iter != prevHypos.end(); ++iter)
	{
		const MosesChart::Hypothesis *prevHypo = *iter;

		VERBOSE(3,prevHypo->GetId() << " <= ");
		Backtrack(prevHypo);
	}
}

void IOWrapper::OutputBestHypo(const std::vector<const Factor*>&  mbrBestHypo, long /*translationId*/, bool reportSegmentation, bool reportAllFactors)
{
	for (size_t i = 0 ; i < mbrBestHypo.size() ; i++)
			{
				const Factor *factor = mbrBestHypo[i];
				cout << *factor << " ";
			}
}
/*
void OutputInput(std::vector<const Phrase*>& map, const MosesChart::Hypothesis* hypo)
{
	if (hypo->GetPrevHypos())
	{
	  	OutputInput(map, hypo->GetPrevHypos());
		map[hypo->GetCurrSourceWordsRange().GetStartPos()] = hypo->GetSourcePhrase();
	}
}

void OutputInput(std::ostream& os, const MosesChart::Hypothesis* hypo)
{
	size_t len = StaticData::Instance().GetInput()->GetSize();
	std::vector<const Phrase*> inp_phrases(len, 0);
	OutputInput(inp_phrases, hypo);
	for (size_t i=0; i<len; ++i)
		if (inp_phrases[i]) os << *inp_phrases[i];
}
*/

void OutputTranslationOptions(std::ostream &out, const MosesChart::Hypothesis *hypo, long translationId)
{ // recursive
	if (hypo != NULL)
	{
	  out << "Trans Opt " << translationId << " " << hypo->GetCurrSourceRange() << ": " <<  hypo->GetCurrChartRule() 
				<< " " << hypo->GetTotalScore() << hypo->GetScoreBreakdown()
				<< endl;
	}
	
	const std::vector<const MosesChart::Hypothesis*> &prevHypos = hypo->GetPrevHypos();
	std::vector<const MosesChart::Hypothesis*>::const_iterator iter;
	for (iter = prevHypos.begin(); iter != prevHypos.end(); ++iter)
	{
		const MosesChart::Hypothesis *prevHypo = *iter;
		OutputTranslationOptions(out, prevHypo, translationId);
	}
}

void IOWrapper::OutputBestHypo(const MosesChart::Hypothesis *hypo, long translationId, bool reportSegmentation, bool reportAllFactors)
{
	if (hypo != NULL)
	{
		VERBOSE(1,"BEST TRANSLATION: " << *hypo << endl);		
		VERBOSE(3,"Best path: ");
		Backtrack(hypo);
		VERBOSE(3,"0" << std::endl);

		if (StaticData::Instance().GetOutputHypoScore())
		{
			cout << hypo->GetTotalScore() << " " 
					<< MosesChart::Hypothesis::GetHypoCount() << " ";
		}
		
		if (StaticData::Instance().IsDetailedTranslationReportingEnabled())
		{
			OutputTranslationOptions(*m_detailedTranslationReportingStream, hypo, translationId);
		}
		
		if (!m_surpressSingleBestOutput)
		{
			if (StaticData::Instance().IsPathRecoveryEnabled()) {
				//OutputInput(cout, hypo);
				cout << "||| ";
			}
			Phrase outPhrase(Output);
			hypo->CreateOutputPhrase(outPhrase);

			// delete 1st & last
			assert(outPhrase.GetSize() >= 2);
			outPhrase.RemoveWord(0);
			outPhrase.RemoveWord(outPhrase.GetSize() - 1);

			const std::vector<FactorType> outputFactorOrder = StaticData::Instance().GetOutputFactorOrder();
			string output = outPhrase.GetStringRep(outputFactorOrder);
			cout << output << endl;
		}
	}
	else
	{
		VERBOSE(1, "NO BEST TRANSLATION" << endl);
		
		if (StaticData::Instance().GetOutputHypoScore())
		{
			cout << "0 ";
		}
		
		if (!m_surpressSingleBestOutput)
		{
			cout << endl;
		}
	}
}

void IOWrapper::OutputNBestList(const MosesChart::TrellisPathList &nBestList, long translationId)
{
	bool labeledOutput = StaticData::Instance().IsLabeledNBestList();
	//bool includeAlignment = StaticData::Instance().NBestIncludesAlignment();

	MosesChart::TrellisPathList::const_iterator iter;
	for (iter = nBestList.begin() ; iter != nBestList.end() ; ++iter)
	{
		const MosesChart::TrellisPath &path = **iter;
		//cerr << path << endl << endl;

		Moses::Phrase outputPhrase = path.GetOutputPhrase();

		// delete 1st & last
		assert(outputPhrase.GetSize() >= 2);
		outputPhrase.RemoveWord(0);
		outputPhrase.RemoveWord(outputPhrase.GetSize() - 1);

		// print the surface factor of the translation
		*m_nBestStream << translationId << " ||| ";
		OutputSurface(*m_nBestStream, outputPhrase, m_outputFactorOrder, false);
		*m_nBestStream << " ||| ";

		// print the scores in a hardwired order
    // before each model type, the corresponding command-line-like name must be emitted
    // MERT script relies on this

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
			vector<PhraseDictionaryFeature*> pds = StaticData::Instance().GetPhraseDictionaries();
			if (pds.size() > 0) {
				if (labeledOutput)
					*m_nBestStream << "tm: ";
				vector<PhraseDictionaryFeature*>::iterator iter;
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
			vector<PhraseDictionaryFeature*> pds = StaticData::Instance().GetPhraseDictionaries();
			if (pds.size() > 0) {
				vector<PhraseDictionaryFeature*>::iterator iter;

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

		/*
    if (includeAlignment) {
			*m_nBestStream << " |||";
			for (int currEdge = (int)edges.size() - 2 ; currEdge >= 0 ; currEdge--)
			{
				const MosesChart::Hypothesis &edge = *edges[currEdge];
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
		*/

    *m_nBestStream << endl;
	}

	*m_nBestStream<<std::flush;
}

