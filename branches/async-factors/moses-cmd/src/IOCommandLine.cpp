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
#include "IOCommandLine.h"
#include "Hypothesis.h"
#include "WordsRange.h"
#include "LatticePathList.h"
#include "StaticData.h"
#include "DummyScoreProducers.h"

using namespace std;

IOCommandLine::IOCommandLine(
				const vector<FactorType>				&inputFactorOrder
				, const vector<FactorType>			&outputFactorOrder
				, const FactorMask							&inputFactorUsed
				, FactorCollection							&factorCollection
				, size_t												nBestSize
				, const string									&nBestFilePath)
:m_inputFactorOrder(inputFactorOrder)
,m_outputFactorOrder(outputFactorOrder)
,m_inputFactorUsed(inputFactorUsed)
,m_factorCollection(factorCollection)
{
	if (nBestSize > 0)
	{
		m_nBestFile.open(nBestFilePath.c_str());
	}
}

InputType*IOCommandLine::GetInput(InputType* in)
{
	return InputOutput::GetInput(in,std::cin,m_inputFactorOrder, m_factorCollection);
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
									 ,bool reportSourceSpan, bool reportAllFactors)
{
	if ( hypo != NULL)
	{
		if (hypo->GetPTID() == -1) OutputSurface(out, hypo->GetPrevHypo(), outputFactorOrder, reportSourceSpan, reportAllFactors);
		OutputSurface(out, hypo->GetTargetPhrase(), outputFactorOrder, reportAllFactors);

        if (reportSourceSpan == true
          && hypo->GetTargetPhrase().GetSize() > 0) {
          out << "|" << hypo->GetCurrSourceWordsRange().GetStartPos()
              << "-" << hypo->GetCurrSourceWordsRange().GetEndPos() << "| ";
        }
	}
}

void IOCommandLine::Backtrack(const Hypothesis *hypo){

	if (hypo->GetPrevHypo() != NULL) {
		TRACE_ERR("["<< hypo ->m_id<<" => "<<hypo->GetPrevHypo()->m_id<<"]" <<endl);
		Backtrack(hypo->GetPrevHypo());
	}
}

void IOCommandLine::SetOutput(const Hypothesis *hypo, long /*translationId*/, bool reportSourceSpan, bool reportAllFactors)
{
	if (hypo != NULL)
	{
		TRACE_ERR("BEST HYPO: " << *hypo << endl);
		TRACE_ERR(hypo->GetScoreBreakdown() << std::endl);
		Backtrack(hypo);

		OutputSurface(cout, hypo, m_outputFactorOrder, reportSourceSpan, reportAllFactors);
	}
	else
	{
		TRACE_ERR("NO BEST HYPO" << endl);
	}
	
	cout << endl;
}

void IOCommandLine::SetNBest(const LatticePathList &nBestList, long translationId)
{
	LatticePathList::const_iterator iter;
	for (iter = nBestList.begin() ; iter != nBestList.end() ; ++iter)
	{
		const LatticePath &path = **iter;
		const std::vector<const Hypothesis *> &edges = path.GetEdges();

		// print the surface factor of the translation
		m_nBestFile << translationId << " ||| ";
		for (int currEdge = (int)edges.size() - 1 ; currEdge >= 0 ; currEdge--)
		{
			const Hypothesis &edge = *edges[currEdge];
			OutputSurface(m_nBestFile, edge.GetTargetPhrase(), m_outputFactorOrder, false); // false for not reporting all factors
		}
		m_nBestFile << " ||| ";

		// print the scores in a hardwired order
    // before each model type, the corresponding command-line-like name must be emitted
    // MERT script relies on this

		// basic distortion
    m_nBestFile << "d: ";
		m_nBestFile << path.GetScoreBreakdown().GetScoreForProducer(StaticData::Instance()->GetDistortionScoreProducer()) << " ";

		// lm
		const LMList& lml = StaticData::Instance()->GetAllLM();
    if (lml.size() > 0) {
      m_nBestFile << "lm: ";
		  LMList::const_iterator lmi = lml.begin();
		  for (; lmi != lml.end(); ++lmi) {
			  m_nBestFile << path.GetScoreBreakdown().GetScoreForProducer(*lmi) << " ";
		  }
    }

		// translation components
		vector<PhraseDictionaryBase*> pds = StaticData::Instance()->GetPhraseDictionaries();
    if (pds.size() > 0) {
      m_nBestFile << "tm: ";
		  vector<PhraseDictionaryBase*>::reverse_iterator i = pds.rbegin();
		  for (; i != pds.rend(); ++i) {
			  vector<float> scores = path.GetScoreBreakdown().GetScoresForProducer(*i);
			  for (size_t j = 0; j<scores.size(); ++j) 
				  m_nBestFile << scores[j] << " ";
			  
		  }
    }
		
		// word penalty
    m_nBestFile << "w: ";
		m_nBestFile << path.GetScoreBreakdown().GetScoreForProducer(StaticData::Instance()->GetWordPenaltyProducer()) << " ";
		
		// generation
		vector<GenerationDictionary*> gds = StaticData::Instance()->GetGenerationDictionaries();
    if (gds.size() > 0) {
      m_nBestFile << "g: ";
		  vector<GenerationDictionary*>::reverse_iterator gi = gds.rbegin();
		  for (; gi != gds.rend(); ++gi) {
			  vector<float> scores = path.GetScoreBreakdown().GetScoresForProducer(*gi);
			  for (size_t j = 0; j<scores.size(); j++) {
				  m_nBestFile << scores[j] << " ";
			  }
		  }
    }
		
		// total						
		m_nBestFile << "||| " << path.GetTotalScore() << endl;
	}

	m_nBestFile<<std::flush;
}
