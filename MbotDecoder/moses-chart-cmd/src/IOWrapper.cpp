// $Id: IOWrapper.cpp,v 1.1.1.1 2013/01/06 16:54:09 braunefe Exp $

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
#include "ChartTrellisPathList.h"
#include "ChartTrellisPath.h"
#include "ChartTranslationOption.h"
#include "ChartHypothesis.h"
#include "DotChart.h"
#include "ProcessedNonTerminals.h"
#include "ChartHypothesisMBOT.h"
#include "ChartTranslationOptionMBOT.h"
#include "GapPenaltyProducer.h"

#include <boost/shared_ptr.hpp>

using namespace std;
using namespace Moses;

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
  ,m_detailedTranslationReportingStream(NULL)
  ,m_detailedAllTranslationReportingStream(NULL)
  ,m_inputFilePath(inputFilePath)
  ,m_detailOutputCollector(NULL)
  ,m_detailAllOutputCollector(NULL)
  ,m_nBestOutputCollector(NULL)
  ,m_searchGraphOutputCollector(NULL)
  ,m_singleBestOutputCollector(NULL)
{
  const StaticData &staticData = StaticData::Instance();

  if (m_inputFilePath.empty()) {
    m_inputStream = &std::cin;
  } else {
    m_inputStream = new InputFileStream(inputFilePath);
  }

  m_surpressSingleBestOutput = false;

  if (nBestSize > 0) {
    if (nBestFilePath == "-") {
      m_nBestStream = &std::cout;
      m_surpressSingleBestOutput = true;
    } else {
      std::ofstream *nBestFile = new std::ofstream;
      m_nBestStream = nBestFile;
      nBestFile->open(nBestFilePath.c_str());
    }
    m_nBestOutputCollector = new Moses::OutputCollector(m_nBestStream);
  }

  if (!m_surpressSingleBestOutput) {
    m_singleBestOutputCollector = new Moses::OutputCollector(&std::cout);
  }

  // search graph output
  if (staticData.GetOutputSearchGraph()) {
    string fileName = staticData.GetParam("output-search-graph")[0];
    std::ofstream *file = new std::ofstream;
    m_outputSearchGraphStream = file;
    file->open(fileName.c_str());
    m_searchGraphOutputCollector = new Moses::OutputCollector(m_outputSearchGraphStream);
  }

  // detailed translation reporting
  if (staticData.IsDetailedTranslationReportingEnabled()) {
    const std::string &path = staticData.GetDetailedTranslationReportingFilePath();
    m_detailedTranslationReportingStream = new std::ofstream(path.c_str());
    m_detailOutputCollector = new Moses::OutputCollector(m_detailedTranslationReportingStream);
  }

  if (staticData.IsDetailedAllTranslationReportingEnabled()) {
      const std::string &path = staticData.GetDetailedAllTranslationReportingFilePath();
      m_detailedAllTranslationReportingStream = new std::ofstream(path.c_str());
      m_detailAllOutputCollector = new Moses::OutputCollector(m_detailedAllTranslationReportingStream);
    }
}

IOWrapper::~IOWrapper()
{
  if (!m_inputFilePath.empty()) {
    delete m_inputStream;
  }
  if (!m_surpressSingleBestOutput) {
    // outputting n-best to file, rather than stdout. need to close file and delete obj
    delete m_nBestStream;
  }
  delete m_outputSearchGraphStream;
  delete m_detailedTranslationReportingStream;
  delete m_detailOutputCollector;
  delete m_detailAllOutputCollector;
  delete m_nBestOutputCollector;
  delete m_searchGraphOutputCollector;
  delete m_singleBestOutputCollector;
}

void IOWrapper::ResetTranslationId() {
  m_translationId = StaticData::Instance().GetStartTranslationId();
}

InputType*IOWrapper::GetInput(InputType* inputType)
{
  if(inputType->Read(*m_inputStream, m_inputFactorOrder)) {
    if (long x = inputType->GetTranslationId()) {
      if (x>=m_translationId) m_translationId = x+1;
    } else inputType->SetTranslationId(m_translationId++);

    return inputType;
  } else {
    delete inputType;
    return NULL;
  }
}

/***
 * print surface factor only for the given phrase
 */
void OutputSurface(std::ostream &out, const Phrase &phrase, const std::vector<FactorType> &outputFactorOrder, bool reportAllFactors)
{
  CHECK(outputFactorOrder.size() > 0);
  if (reportAllFactors == true) {
    out << phrase;
  } else {
    size_t size = phrase.GetSize();
    for (size_t pos = 0 ; pos < size ; pos++) {
      const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[0]);
      out << *factor;

      for (size_t i = 1 ; i < outputFactorOrder.size() ; i++) {
        const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[i]);
        out << "|" << *factor;
      }
      out << " ";
    }
  }
}

void OutputSurface(std::ostream &out, const ChartHypothesis *hypo, const std::vector<FactorType> &outputFactorOrder
                   ,bool reportSegmentation, bool reportAllFactors)
{
  if ( hypo != NULL) {
    //OutputSurface(out, hypo->GetCurrTargetPhrase(), outputFactorOrder, reportAllFactors);

    const vector<const ChartHypothesis*> &prevHypos = hypo->GetPrevHypos();

    vector<const ChartHypothesis*>::const_iterator iter;
    for (iter = prevHypos.begin(); iter != prevHypos.end(); ++iter) {
      const ChartHypothesis *prevHypo = *iter;

      OutputSurface(out, prevHypo, outputFactorOrder, reportSegmentation, reportAllFactors);
    }
  }
}

void IOWrapper::Backtrack(const ChartHypothesis *hypo)
{
  const vector<const ChartHypothesis*> &prevHypos = hypo->GetPrevHypos();

  vector<const ChartHypothesis*>::const_iterator iter;
  for (iter = prevHypos.begin(); iter != prevHypos.end(); ++iter) {
    const ChartHypothesis *prevHypo = *iter;

    VERBOSE(3,prevHypo->GetId() << " <= ");
    Backtrack(prevHypo);
  }
}

void IOWrapper::OutputBestHypo(const std::vector<const Factor*>&  mbrBestHypo, long /*translationId*/, bool /* reportSegmentation */, bool /* reportAllFactors */)
{
  for (size_t i = 0 ; i < mbrBestHypo.size() ; i++) {
    const Factor *factor = mbrBestHypo[i];
    cout << *factor << " ";
  }
}

void OutputTranslationOptions(std::ostream &out, const ChartHypothesis *hypo, long translationId)
{
  // recursive
  if (hypo != NULL) {
    out << "Trans Opt " << translationId
        << " " << hypo->GetCurrSourceRange()
        << ": " << hypo->GetTranslationOption().GetDottedRule()
        << ": " << hypo->GetCurrTargetPhrase().GetTargetLHS()
        << "->" << hypo->GetCurrTargetPhrase()
        << " " << hypo->GetTotalScore() << hypo->GetScoreBreakdown()
        << endl;
  }

  const std::vector<const ChartHypothesis*> &prevHypos = hypo->GetPrevHypos();
  std::vector<const ChartHypothesis*>::const_iterator iter;
  for (iter = prevHypos.begin(); iter != prevHypos.end(); ++iter) {
    const ChartHypothesis *prevHypo = *iter;
    OutputTranslationOptions(out, prevHypo, translationId);
  }
}

//new : output translation options for mbot
void OutputTranslationOptionsMBOT(std::ostream &out, const ChartHypothesisMBOT *hypo, long translationId)
{
  // recursive
  if (hypo != NULL) {

    out << "Trans Opt " << translationId
        << " " << hypo->GetCurrSourceRange()
        << ": " << hypo->GetTranslationOptionMBOT().GetDottedRuleMBOT()
        //LOOK more precisely at targetLHSMBOT
        //<< ": " << hypo->GetCurrTargetPhraseMBOT().GetTargetLHSMBOT()
        << "->" << hypo->GetCurrTargetPhraseMBOT()
        << " " << hypo->GetTotalScore() << hypo->GetScoreBreakdown()
        << endl;
  }

  const std::vector<const ChartHypothesisMBOT*> &prevHypos = hypo->GetPrevHyposMBOT();
  std::vector<const ChartHypothesisMBOT*>::const_iterator iter;
  for (iter = prevHypos.begin(); iter != prevHypos.end(); ++iter) {
    const ChartHypothesisMBOT *prevHypo = *iter;
    OutputTranslationOptionsMBOT(out, prevHypo, translationId);
  }
}


void IOWrapper::OutputDetailedTranslationReport(
  const ChartHypothesis *hypo,
  long translationId)
{
  if (hypo == NULL) {
    return;
  }
  std::ostringstream out;
  OutputTranslationOptions(out, hypo, translationId);
  CHECK(m_detailOutputCollector);
  m_detailOutputCollector->Write(translationId, out.str());
}

//output translation report for mbot rules
void IOWrapper::OutputDetailedTranslationReportMBOT(
  const ChartHypothesisMBOT *hypo,
  long translationId)
{
  if (hypo == NULL) {
  //VERBOSE(2,"NO HYPO" << endl);
    return;
  }
  std::ostringstream out;
  OutputTranslationOptionsMBOT(out, hypo, translationId);
 // VERBOSE(2,"TRACE REPORT : " << out.str() << endl);
  CHECK(m_detailOutputCollector);
  m_detailOutputCollector->Write(translationId, out.str());
}

void IOWrapper::OutputDetailedAllTranslationReportMBOT(
  const ChartManager &manager,
  long translationId)
{
	std::ostringstream out;

  const ChartCellCollection& cells = manager.GetChartCellCollection();
  size_t size = manager.GetSource().GetSize();
  for (size_t width = 1; width <= size; ++width) {
    for (size_t startPos = 0; startPos <= size-width; ++startPos) {
      size_t endPos = startPos + width - 1;
      WordsRange range(startPos, endPos);
      const ChartCell& cell = cells.Get(range);
      const HypoList* hyps = cell.GetAllSortedHypotheses();
      out << "Chart Cell [" << startPos << ".." << endPos << "]" << endl;
      HypoList::const_iterator iter;
      size_t c = 1;
      for (iter = hyps->begin(); iter != hyps->end(); ++iter) {
	out << "----------------Item " << c++ << " ---------------------"
	    << endl;
		const ChartHypothesis *hypo_const = *iter;
		ChartHypothesis *hypo = const_cast<ChartHypothesis*>(hypo_const);
		ChartHypothesisMBOT * mbotHypo = static_cast<ChartHypothesisMBOT*>(hypo);
		OutputTranslationOptionsMBOT(out, mbotHypo, translationId);
      }
    }
  }
  CHECK(m_detailAllOutputCollector);
  m_detailAllOutputCollector->Write(translationId, out.str());
}

void IOWrapper::OutputBestHypo(const ChartHypothesis *hypo, long translationId, bool /* reportSegmentation */, bool /* reportAllFactors */)
{
  std::ostringstream out;
  IOWrapper::FixPrecision(out);
  if (hypo != NULL) {
    VERBOSE(1,"BEST TRANSLATION: " << *hypo << endl);
    VERBOSE(3,"Best path: ");
    Backtrack(hypo);
    VERBOSE(3,"0" << std::endl);

    if (StaticData::Instance().GetOutputHypoScore()) {
      out << hypo->GetTotalScore() << " ";
    }

    if (!m_surpressSingleBestOutput) {
      if (StaticData::Instance().IsPathRecoveryEnabled()) {
        out << "||| ";
      }
      Phrase outPhrase(ARRAY_SIZE_INCR);

      hypo->CreateOutputPhrase(outPhrase);

      // delete 1st & last
      CHECK(outPhrase.GetSize() >= 2);
      outPhrase.RemoveWord(0);
      outPhrase.RemoveWord(outPhrase.GetSize() - 1);

      const std::vector<FactorType> outputFactorOrder = StaticData::Instance().GetOutputFactorOrder();
      string output = outPhrase.GetStringRep(outputFactorOrder);
      out << output << endl;
    }
  } else {
    VERBOSE(1, "NO BEST TRANSLATION" << endl);

    if (StaticData::Instance().GetOutputHypoScore()) {
      out << "0 ";
    }

    out << endl;
  }

  if (m_singleBestOutputCollector) {
    m_singleBestOutputCollector->Write(translationId, out.str());
  }
}

//new : output best hypothesis for MBOT Hypotheses
void IOWrapper::OutputBestHypoMBOT(string sourceSent, const ChartHypothesisMBOT *hypo, long translationId, bool /* reportSegmentation */, bool /* reportAllFactors */)
{
  std::ostringstream out;
  IOWrapper::FixPrecision(out);
  if (hypo != NULL) {
    VERBOSE(1,"BEST TRANSLATION: " << *hypo << endl);
    VERBOSE(3,"Best path: ");
    Backtrack(hypo);
    VERBOSE(3,"0" << std::endl);

    if (StaticData::Instance().GetOutputHypoScore()) {
      out << hypo->GetTotalScore() << " ";
    }

    if (!m_surpressSingleBestOutput) {
      if (StaticData::Instance().IsPathRecoveryEnabled()) {
        out << "||| ";
      }
      Phrase outPhrase(ARRAY_SIZE_INCR);

      //Store processed non terminals due to multiple target sides
      ProcessedNonTerminals * processedNonTerms = new ProcessedNonTerminals();

      //reset status for all Hypos
      hypo->CreateOutputPhrase(outPhrase, *processedNonTerms);

      // delete 1st & last
      CHECK(outPhrase.GetSize() >= 2);
      outPhrase.RemoveWord(0);
      outPhrase.RemoveWord(outPhrase.GetSize() - 1);

      const std::vector<FactorType> outputFactorOrder = StaticData::Instance().GetOutputFactorOrder();
      string output = outPhrase.GetStringRep(outputFactorOrder);
      out << output << endl;
      //VERBOSE(1,"BEST TRANSLATION STRING: " << output << endl);
      //write output into file
      //std::cout << output << std::endl;
      delete processedNonTerms;
    }
  } else {
    VERBOSE(1, "NO BEST TRANSLATION" << endl);
    //new : also cout that no translation has been found
    //out << sourceSent;
    out << "FAILED";

    if (StaticData::Instance().GetOutputHypoScore()) {
      out << "0 ";
    }
    out << endl;
  }

  if (m_singleBestOutputCollector) {
    m_singleBestOutputCollector->Write(translationId, out.str());
  }
}

void IOWrapper::OutputNBestList(const ChartTrellisPathList &nBestList, const ChartHypothesis *bestHypo, const TranslationSystem* system, long translationId)
{
  std::ostringstream out;

  // Check if we're writing to std::cout.
  if (m_surpressSingleBestOutput) {
    // Set precision only if we're writing the n-best list to cout.  This is to
    // preserve existing behaviour, but should probably be done either way.
    IOWrapper::FixPrecision(out);

    // The output from -output-hypo-score is always written to std::cout.
    if (StaticData::Instance().GetOutputHypoScore()) {
      if (bestHypo != NULL) {
        out << bestHypo->GetTotalScore() << " ";
      } else {
        out << "0 ";
      }
    }
  }

  bool labeledOutput = StaticData::Instance().IsLabeledNBestList();
  //bool includeAlignment = StaticData::Instance().NBestIncludesAlignment();

  ChartTrellisPathList::const_iterator iter;
  for (iter = nBestList.begin() ; iter != nBestList.end() ; ++iter) {
    const ChartTrellisPath &path = **iter;
    //cerr << path << endl << endl;

    Moses::Phrase outputPhrase = path.GetOutputPhrase();

    // delete 1st & last
    CHECK(outputPhrase.GetSize() >= 2);
    outputPhrase.RemoveWord(0);
    outputPhrase.RemoveWord(outputPhrase.GetSize() - 1);

    // print the surface factor of the translation
    out << translationId << " ||| ";
    OutputSurface(out, outputPhrase, m_outputFactorOrder, false);
    out << " |||";

    // print the scores in a hardwired order
    // before each model type, the corresponding command-line-like name must be emitted
    // MERT script relies on this

    // lm
    const LMList& lml = system->GetLanguageModels();
    if (lml.size() > 0) {
      if (labeledOutput)
        out << "lm:";
      LMList::const_iterator lmi = lml.begin();
      for (; lmi != lml.end(); ++lmi) {
        out << " " << path.GetScoreBreakdown().GetScoreForProducer(*lmi);
      }
    }


    std::string lastName = "";

    // translation components
    const vector<PhraseDictionaryFeature*>& pds = system->GetPhraseDictionaries();
    if (pds.size() > 0) {

      for( size_t i=0; i<pds.size(); i++ ) {
	size_t pd_numinputscore = pds[i]->GetNumInputScores();
	vector<float> scores = path.GetScoreBreakdown().GetScoresForProducer( pds[i] );
	for (size_t j = 0; j<scores.size(); ++j){

	  if (labeledOutput && (i == 0) ){
	    if ((j == 0) || (j == pd_numinputscore)){
	      lastName =  pds[i]->GetScoreProducerWeightShortName(j);
	      out << " " << lastName << ":";
	    }
	  }
	  out << " " << scores[j];
	}
      }
    }

    // word penalty
    if (labeledOutput)
      out << " w:";
    out << " " << path.GetScoreBreakdown().GetScoreForProducer(system->GetWordPenaltyProducer());

    // generation
    const vector<GenerationDictionary*>& gds = system->GetGenerationDictionaries();
    if (gds.size() > 0) {

      for( size_t i=0; i<gds.size(); i++ ) {
	size_t pd_numinputscore = gds[i]->GetNumInputScores();
	vector<float> scores = path.GetScoreBreakdown().GetScoresForProducer( gds[i] );
	for (size_t j = 0; j<scores.size(); ++j){

	  if (labeledOutput && (i == 0) ){
	    if ((j == 0) || (j == pd_numinputscore)){
	      lastName =  gds[i]->GetScoreProducerWeightShortName(j);
	      out << " " << lastName << ":";
	    }
	  }
	  out << " " << scores[j];
	}
      }
    }

    // total
    out << " |||" << path.GetTotalScore();

    out << endl;
  }

  out <<std::flush;

  CHECK(m_nBestOutputCollector);
  m_nBestOutputCollector->Write(translationId, out.str());
}

//Output n-best list for MBOT hypos
void IOWrapper::OutputNBestListMBOT(string sourceSent, const ChartTreillisPathListMBOT &nBestList, const ChartHypothesisMBOT *bestHypo, const TranslationSystem* system, long translationId, Moses::ProcessedNonTerminals * nt)
{
  std::ostringstream out;

  // Check if we're writing to std::cout.
  if (m_surpressSingleBestOutput) {
    // Set precision only if we're writing the n-best list to cout.  This is to
    // preserve existing behaviour, but should probably be done either way.
    IOWrapper::FixPrecision(out);

    // The output from -output-hypo-score is always written to std::cout.
    if (StaticData::Instance().GetOutputHypoScore()) {
      if (bestHypo != NULL) {
        out << bestHypo->GetTotalScore() << " ";
      } else {
        out << "0 ";
      }
    }
  }

  bool labeledOutput = StaticData::Instance().IsLabeledNBestList();
  //bool includeAlignment = StaticData::Instance().NBestIncludesAlignment();

  //Handling for empty hypos
 /* if(bestHypo == NULL)
  {
	  out << translationId << " ||| ";
	  out << sourceSent;
	  out << " ||| ";
	  out << "lm: 0 tm: 0 0 0 0 0 w: 0 gpp: 0";
	  out << " ||| ";
	  out << "0";
	  out << endl;
  }
  else
  {*/

  ChartTreillisPathListMBOT::const_iterator iter;
  for (iter = nBestList.begin() ; iter != nBestList.end() ; ++iter) {
    const ChartTreillisPathMBOT &path = **iter;
    //cerr << path << endl << endl;
    nt->Reset();
    Moses::Phrase outputPhrase = path.GetOutputPhraseMBOT(nt);

    // delete 1st & last
    CHECK(outputPhrase.GetSize() >= 2);
    outputPhrase.RemoveWord(0);
    outputPhrase.RemoveWord(outputPhrase.GetSize() - 1);

    // print the surface factor of the translation
    out << translationId << " ||| ";
    OutputSurface(out, outputPhrase, m_outputFactorOrder, false);
    out << " |||";

    // print the scores in a hardwired order
    // before each model type, the corresponding command-line-like name must be emitted
    // MERT script relies on this

    // lm
    const LMList& lml = system->GetLanguageModels();
    if (lml.size() > 0) {
      if (labeledOutput)
        out << "lm:";
      LMList::const_iterator lmi = lml.begin();
      for (; lmi != lml.end(); ++lmi) {
        out << " " << path.GetScoreBreakdownMBOT().GetScoreForProducer(*lmi);
      }
    }
    std::string lastName = "";

    // translation components
    const vector<PhraseDictionaryFeature*>& pds = system->GetPhraseDictionaries();
    if (pds.size() > 0) {

      for( size_t i=0; i<pds.size(); i++ ) {
	size_t pd_numinputscore = pds[i]->GetNumInputScores();
	vector<float> scores = path.GetScoreBreakdownMBOT().GetScoresForProducer( pds[i] );
	for (size_t j = 0; j<scores.size(); ++j){

	  if (labeledOutput && (i == 0) ){
	    if ((j == 0) || (j == pd_numinputscore)){
	      lastName =  pds[i]->GetScoreProducerWeightShortName(j);
	      out << " " << lastName << ":";
	    }
	  }
	  out << " " << scores[j];
	}
      }
    }

    // word penalty
    if (labeledOutput)
      out << " w:";
    out << " " << path.GetScoreBreakdownMBOT().GetScoreForProducer(system->GetWordPenaltyProducer());

    // generation
    const vector<GenerationDictionary*>& gds = system->GetGenerationDictionaries();
    if (gds.size() > 0) {

      for( size_t i=0; i<gds.size(); i++ ) {
	size_t pd_numinputscore = gds[i]->GetNumInputScores();
	vector<float> scores = path.GetScoreBreakdownMBOT().GetScoresForProducer( gds[i] );
	for (size_t j = 0; j<scores.size(); ++j){

	  if (labeledOutput && (i == 0) ){
	    if ((j == 0) || (j == pd_numinputscore)){
	      lastName =  gds[i]->GetScoreProducerWeightShortName(j);
	      out << " " << lastName << ":";
	    }
	  }
	  out << " " << scores[j];
	}
      }
    }

    //Gap penalty producer
          GapPenaltyProducer *gppProducer = StaticData::Instance().GetGapPenaltyProducer();
          if (gppProducer != NULL) {
            out << " " << gppProducer->GetScoreProducerWeightShortName(0) << ":";
            vector<float> scores = path.GetScoreBreakdownMBOT().GetScoresForProducer(gppProducer);
            for (size_t j = 0; j<scores.size(); ++j) {
              out << " " << scores[j];
            }
          }

    // total
    out << " |||" << path.GetTotalScoreMBOT();

    out << endl;
  }
// } End of else
  out <<std::flush;

  CHECK(m_nBestOutputCollector);
  m_nBestOutputCollector->Write(translationId, out.str());
}

void IOWrapper::FixPrecision(std::ostream &stream, size_t size)
{
  stream.setf(std::ios::fixed);
  stream.precision(size);
}
