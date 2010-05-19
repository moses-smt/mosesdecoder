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
#include "Hypothesis.h"
#include "WordsRange.h"
#include "TrellisPathList.h"
#include "StaticData.h"
#include "DummyScoreProducers.h"
#include "InputFileStream.h"

using namespace std;
using namespace Moses;

IOWrapper::IOWrapper(
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
,m_outputWordGraphStream(NULL)
,m_outputSearchGraphStream(NULL)
,m_detailedTranslationReportingStream(NULL)
{
	Initialization(inputFactorOrder, outputFactorOrder
								, inputFactorUsed
								, nBestSize, nBestFilePath);
}

IOWrapper::IOWrapper(const std::vector<FactorType>	&inputFactorOrder
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
,m_outputWordGraphStream(NULL)
,m_outputSearchGraphStream(NULL)
,m_detailedTranslationReportingStream(NULL)
{
	Initialization(inputFactorOrder, outputFactorOrder
								, inputFactorUsed
								, nBestSize, nBestFilePath);

	m_inputStream = m_inputFile;
}

IOWrapper::~IOWrapper()
{
	if (m_inputFile != NULL)
		delete m_inputFile;
	if (m_nBestStream != NULL && !m_surpressSingleBestOutput)
	{ // outputting n-best to file, rather than stdout. need to close file and delete obj
		delete m_nBestStream;
	}
	if (m_outputWordGraphStream != NULL)
	{
		delete m_outputWordGraphStream;
	}
	if (m_outputSearchGraphStream != NULL)
	{
	  delete m_outputSearchGraphStream;
	}
  delete m_detailedTranslationReportingStream;
}

void IOWrapper::Initialization(const std::vector<FactorType>	&/*inputFactorOrder*/
														, const std::vector<FactorType>			&/*outputFactorOrder*/
														, const FactorMask							&/*inputFactorUsed*/
														, size_t												nBestSize
														, const std::string							&nBestFilePath)
{
	const StaticData &staticData = StaticData::Instance();

	// n-best
	m_surpressSingleBestOutput = false;
       
	if (nBestSize > 0)
        {
                if (nBestFilePath == "-" || nBestFilePath == "/dev/stdout")
                {
                        m_nBestStream = &std::cout;
                        m_surpressSingleBestOutput = true;
                } 
                else 
                {
                        std::ofstream *file = new std::ofstream;
                        m_nBestStream = file;
                        file->open(nBestFilePath.c_str());
                }
        }

	// wordgraph output
	if (staticData.GetOutputWordGraph())
	{
		string fileName = staticData.GetParam("output-word-graph")[0];
		std::ofstream *file = new std::ofstream;
		m_outputWordGraphStream  = file;
		file->open(fileName.c_str());
	}

	// search graph output
	if (staticData.GetOutputSearchGraph())
	{
		string fileName;
		if (staticData.GetOutputSearchGraphExtended())
			fileName = staticData.GetParam("output-search-graph-extended")[0];
		else
			fileName = staticData.GetParam("output-search-graph")[0];
	  std::ofstream *file = new std::ofstream;
	  m_outputSearchGraphStream = file;
	  file->open(fileName.c_str());
	}

  // detailed translation reporting
  if (staticData.IsDetailedTranslationReportingEnabled())
  {
    const std::string &path = staticData.GetDetailedTranslationReportingFilePath();
    m_detailedTranslationReportingStream = new std::ofstream(path.c_str());
    assert(m_detailedTranslationReportingStream->good());
  }
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

void OutputBestHypo(const Moses::TrellisPath &path, long /*translationId*/,bool reportSegmentation, bool reportAllFactors, std::ostream &out) 
{	
	const std::vector<const Hypothesis *> &edges = path.GetEdges();

	for (int currEdge = (int)edges.size() - 1 ; currEdge >= 0 ; currEdge--)
	{
		const Hypothesis &edge = *edges[currEdge];
		OutputSurface(out, edge.GetCurrTargetPhrase(), StaticData::Instance().GetOutputFactorOrder(), reportAllFactors);
		if (reportSegmentation == true
		    && edge.GetCurrTargetPhrase().GetSize() > 0) {
			out << "|" << edge.GetCurrSourceWordsRange().GetStartPos()
			    << "-" << edge.GetCurrSourceWordsRange().GetEndPos() << "| ";
		}
	}
  out << endl;
}

void IOWrapper::Backtrack(const Hypothesis *hypo){

	if (hypo->GetPrevHypo() != NULL) {
		VERBOSE(3,hypo->GetId() << " <= ");
		Backtrack(hypo->GetPrevHypo());
	}
}
				
void OutputBestHypo(const std::vector<Word>&  mbrBestHypo, long /*translationId*/, bool /*reportSegmentation*/, bool /*reportAllFactors*/, ostream& out)
{
  
	for (size_t i = 0 ; i < mbrBestHypo.size() ; i++)
	{
        const Factor *factor = mbrBestHypo[i].GetFactor(StaticData::Instance().GetOutputFactorOrder()[0]);
	    if (i>0) out << " ";
            out << *factor;
	}
	out << endl;
}		


void OutputInput(std::vector<const Phrase*>& map, const Hypothesis* hypo)
{
	if (hypo->GetPrevHypo())
	{
	  	OutputInput(map, hypo->GetPrevHypo());
		map[hypo->GetCurrSourceWordsRange().GetStartPos()] = hypo->GetSourcePhrase();
	}
}

void OutputInput(std::ostream& os, const Hypothesis* hypo)
{
	size_t len = hypo->GetInput().GetSize();
	std::vector<const Phrase*> inp_phrases(len, 0);
	OutputInput(inp_phrases, hypo);
	for (size_t i=0; i<len; ++i)
		if (inp_phrases[i]) os << *inp_phrases[i];
}

void IOWrapper::OutputBestHypo(const Hypothesis *hypo, long /*translationId*/, bool reportSegmentation, bool reportAllFactors)
{
	if (hypo != NULL)
	{
		VERBOSE(1,"BEST TRANSLATION: " << *hypo << endl);
		VERBOSE(3,"Best path: ");
		Backtrack(hypo);
		VERBOSE(3,"0" << std::endl);
		if (!m_surpressSingleBestOutput)
		{
			if (StaticData::Instance().IsPathRecoveryEnabled()) {
				OutputInput(cout, hypo);
				cout << "||| ";
			}
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




void OutputNBest(std::ostream& out, const Moses::TrellisPathList &nBestList, const std::vector<Moses::FactorType>& outputFactorOrder,long translationId)
{
	const StaticData &staticData = StaticData::Instance();
	bool labeledOutput = staticData.IsLabeledNBestList();
	bool reportAllFactors = staticData.GetReportAllFactorsNBest();
	bool includeAlignment = staticData.NBestIncludesAlignment();
	//bool includeWordAlignment = staticData.PrintAlignmentInfoInNbest();
	
	TrellisPathList::const_iterator iter;
	for (iter = nBestList.begin() ; iter != nBestList.end() ; ++iter)
	{
		const TrellisPath &path = **iter;
		const std::vector<const Hypothesis *> &edges = path.GetEdges();

		// print the surface factor of the translation
		out << translationId << " ||| ";
		for (int currEdge = (int)edges.size() - 1 ; currEdge >= 0 ; currEdge--)
		{
			const Hypothesis &edge = *edges[currEdge];
			OutputSurface(out, edge.GetCurrTargetPhrase(), outputFactorOrder, reportAllFactors);
		}
		out << " |||";

		std::string lastName = "";
		const vector<const StatefulFeatureFunction*>& sff =
			staticData.GetScoreIndexManager().GetStatefulFeatureFunctions();
		for( size_t i=0; i<sff.size(); i++ )
		{
			if( labeledOutput && lastName != sff[i]->GetScoreProducerWeightShortName() )
			{
				lastName = sff[i]->GetScoreProducerWeightShortName();
				out << " " << lastName << ":";
			}
			vector<float> scores = path.GetScoreBreakdown().GetScoresForProducer( sff[i] );
			for (size_t j = 0; j<scores.size(); ++j) 
			{
		  		out << " " << scores[j];
			}
		}

		const vector<const StatelessFeatureFunction*>& slf =
			staticData.GetScoreIndexManager().GetStatelessFeatureFunctions();
		for( size_t i=0; i<slf.size(); i++ )
		{
			if( labeledOutput && lastName != slf[i]->GetScoreProducerWeightShortName() )
			{
				lastName = slf[i]->GetScoreProducerWeightShortName();
				out << " " << lastName << ":";
			}
			vector<float> scores = path.GetScoreBreakdown().GetScoresForProducer( slf[i] );
			for (size_t j = 0; j<scores.size(); ++j) 
			{
		  		out << " " << scores[j];
			}
		}

		// translation components
		if (StaticData::Instance().GetInputType()==SentenceInput){  
			// translation components	for text input
			vector<PhraseDictionaryFeature*> pds = StaticData::Instance().GetPhraseDictionaries();
			if (pds.size() > 0) {
				if (labeledOutput)
					out << " tm:";
				vector<PhraseDictionaryFeature*>::iterator iter;
				for (iter = pds.begin(); iter != pds.end(); ++iter) {
					vector<float> scores = path.GetScoreBreakdown().GetScoresForProducer(*iter);
					for (size_t j = 0; j<scores.size(); ++j) 
						out << " " << scores[j];
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
						out << " I:";

					for (size_t j = 0; j < pd_numinputscore; ++j)
						out << " " << scores[j];
				}
					
					
				for (iter = pds.begin() ; iter != pds.end(); ++iter) {
					vector<float> scores = path.GetScoreBreakdown().GetScoresForProducer(*iter);
					
					size_t pd_numinputscore = (*iter)->GetNumInputScores();

					if (iter == pds.begin() && labeledOutput)
						out << " tm:";
					for (size_t j = pd_numinputscore; j < scores.size() ; ++j)
						out << " " << scores[j];
				}
			}
		}
		
		// generation
		vector<GenerationDictionary*> gds = StaticData::Instance().GetGenerationDictionaries();
		if (gds.size() > 0) {
			if (labeledOutput)
				out << " g: ";
			vector<GenerationDictionary*>::iterator iter;
			for (iter = gds.begin(); iter != gds.end(); ++iter) {
				vector<float> scores = path.GetScoreBreakdown().GetScoresForProducer(*iter);
				for (size_t j = 0; j<scores.size(); j++) {
					out << scores[j] << " ";
				}
			}
		}
		
		// total						
		out << " ||| " << path.GetTotalScore();
		
		//phrase-to-phrase alignment
		if (includeAlignment) {
			out << " |||";
			for (int currEdge = (int)edges.size() - 2 ; currEdge >= 0 ; currEdge--)
			{
				const Hypothesis &edge = *edges[currEdge];
				const WordsRange &sourceRange = edge.GetCurrSourceWordsRange();
				WordsRange targetRange = path.GetTargetWordsRange(edge);
				out << " " << sourceRange.GetStartPos();
				if (sourceRange.GetStartPos() < sourceRange.GetEndPos()) {
					out << "-" << sourceRange.GetEndPos();
				}
				out<< "=" << targetRange.GetStartPos();
				if (targetRange.GetStartPos() < targetRange.GetEndPos()) {
					out<< "-" << targetRange.GetEndPos();
				}
			}
    		}
	
                if (StaticData::Instance().IsPathRecoveryEnabled()) {
                	out << "|||";
                        OutputInput(out, edges[0]);
                }
				
		out << endl;
	}


	out <<std::flush;
}

void OutputLatticeMBRNBest(std::ostream& out, const vector<LatticeMBRSolution>& solutions,long translationId) {
    for (vector<LatticeMBRSolution>::const_iterator si = solutions.begin(); si != solutions.end(); ++si) {
        out << translationId;
        out << " ||| ";
        const vector<Word> mbrHypo = si->GetWords();
        for (size_t i = 0 ; i < mbrHypo.size() ; i++)
        {
            const Factor *factor = mbrHypo[i].GetFactor(StaticData::Instance().GetOutputFactorOrder()[0]);
            if (i>0) out << " ";
            out << *factor;
        }
        out << " ||| ";
        out << "map: " << si->GetMapScore();
        out << " w: " << mbrHypo.size();
        const vector<float>& ngramScores = si->GetNgramScores();
        for (size_t i = 0; i < ngramScores.size(); ++i) {
            out << " " << ngramScores[i];
        }
        out << " ||| ";
        out << si->GetScore();
        
        out << endl;
    }
}

void IOWrapper::OutputNBestList(const TrellisPathList &nBestList, long translationId) {
    OutputNBest(*m_nBestStream, nBestList,m_outputFactorOrder, translationId);
}

void IOWrapper::OutputLatticeMBRNBestList(const vector<LatticeMBRSolution>& solutions,long translationId) {
    OutputLatticeMBRNBest(*m_nBestStream, solutions,translationId);
}

bool ReadInput(IOWrapper &ioWrapper, InputTypeEnum inputType, InputType*& source)
{
	delete source;
	switch(inputType)
	{
		case SentenceInput:         source = ioWrapper.GetInput(new Sentence(Input)); break;
		case ConfusionNetworkInput: source = ioWrapper.GetInput(new ConfusionNet);    break;
		case WordLatticeInput:      source = ioWrapper.GetInput(new WordLattice);     break;
		default: TRACE_ERR("Unknown input type: " << inputType << "\n");
	}
	return (source ? true : false);
}



IOWrapper *GetIODevice(const StaticData &staticData)
{
	IOWrapper *ioWrapper;
	const std::vector<FactorType> &inputFactorOrder = staticData.GetInputFactorOrder()
																,&outputFactorOrder = staticData.GetOutputFactorOrder();
	FactorMask inputFactorUsed(inputFactorOrder);

	// io
	if (staticData.GetParam("input-file").size() == 1)
	{
	  VERBOSE(2,"IO from File" << endl);
		string filePath = staticData.GetParam("input-file")[0];

		ioWrapper = new IOWrapper(inputFactorOrder, outputFactorOrder, inputFactorUsed
																	, staticData.GetNBestSize()
																	, staticData.GetNBestFilePath()
																	, filePath);
	}
	else
	{
	  VERBOSE(1,"IO from STDOUT/STDIN" << endl);
		ioWrapper = new IOWrapper(inputFactorOrder, outputFactorOrder, inputFactorUsed
																	, staticData.GetNBestSize()
																	, staticData.GetNBestFilePath());
	}
	ioWrapper->ResetTranslationId();

	IFVERBOSE(1)
		PrintUserTime("Created input-output object");

	return ioWrapper;
}
