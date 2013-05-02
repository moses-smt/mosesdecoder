// $Id: IOWrapper.cpp,v 1.1 2012/10/07 13:43:06 braunefe Exp $

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
#include "CellContextScoreProducer.h"

using namespace std;
using namespace Moses;

namespace MosesChartCmd
{

IOWrapper::IOWrapper(const std::vector<FactorType>	&inputFactorOrder
                     , const std::vector<FactorType>	&outputFactorOrder
                     , const FactorMask							&inputFactorUsed
                     , size_t												nBestSize
                     , const std::string							&nBestFilePath
                     , const std::string							&inputFilePath
                     //damt hiero : psd context file path
                     , const std::string &contextFilePath
                     //damt hiero : parse context file path
                     , const std::string &parseFilePath)
  :m_inputFactorOrder(inputFactorOrder)
  ,m_outputFactorOrder(outputFactorOrder)
  ,m_inputFactorUsed(inputFactorUsed)
  ,m_nBestStream(NULL)
  ,m_outputSearchGraphStream(NULL)
  ,m_detailedTranslationReportingStream(NULL)
  ,m_inputFilePath(inputFilePath)
  ,m_detailOutputCollector(NULL)
  ,m_nBestOutputCollector(NULL)
  ,m_searchGraphOutputCollector(NULL)
  ,m_singleBestOutputCollector(NULL)
  //damt hiero : input containing psd context
  ,m_contextFilePath(contextFilePath)
  //damt hiero : input containing parse
  ,m_parseFilePath(parseFilePath)
{
  const StaticData &staticData = StaticData::Instance();

  if (m_inputFilePath.empty()) {
    m_inputStream = &std::cin;
  } else {
    m_inputStream = new InputFileStream(inputFilePath);
  }

  //damt hiero : psd context file
  if (m_contextFilePath.empty()) {
    m_contextStream = &std::cin;
  } else {
    m_contextStream = new InputFileStream(contextFilePath);
  }

  //damt hiero : psd parse file
  if (m_parseFilePath.empty()) {
     m_parseStream = &std::cin;
   } else {
     m_parseStream = new InputFileStream(parseFilePath);
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
	VERBOSE(2,"CREATING OUTPUT COLLECTOR" << endl);
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
  // word alignment reporting
  if (staticData.IsAlignmentReportingEnabled()) {
        const std::string &alignFile = staticData.GetAlignmentOutputFile();
        m_wordAlignmentStream = new std::ofstream(alignFile.c_str());
        m_wordAlignmentOutputCollector = new Moses::OutputCollector(m_wordAlignmentStream);
  }
}

IOWrapper::~IOWrapper()
{
  if (!m_inputFilePath.empty()) {
    delete m_inputStream;
  }

  //damt hiero : psd context file
  if (!m_contextFilePath.empty()) {
    delete m_contextStream;
  }

  //damt hiero : file to parse
  if (!m_parseFilePath.empty()) {
      delete m_parseStream;
    }

  if (!m_surpressSingleBestOutput) {
    // outputting n-best to file, rather than stdout. need to close file and delete obj
    delete m_nBestStream;
  }
  delete m_outputSearchGraphStream;
  delete m_detailedTranslationReportingStream;
  delete m_detailOutputCollector;
  delete m_nBestOutputCollector;
  delete m_searchGraphOutputCollector;
  delete m_singleBestOutputCollector;

  //damt hiero : maybe check for option
  if(m_wordAlignmentStream != NULL)
  {delete m_wordAlignmentStream;}
  if(m_wordAlignmentOutputCollector != NULL)
  {delete m_wordAlignmentOutputCollector;}
}

void IOWrapper::ResetTranslationId() {
  m_translationId = StaticData::Instance().GetStartTranslationId();
}

InputType*IOWrapper::GetInput(InputType* inputType)
{

  if(inputType->Read(*m_inputStream, m_inputFactorOrder)) {
    if (long x = inputType->GetTranslationId()) {
      if (x>=m_translationId) m_translationId = x+1;
    } else
    	inputType->SetTranslationId(m_translationId++);

    //damt hiero : also read context file
    if(!ReadContext(*m_contextStream, inputType))
    {
    	cerr << "Warning : If using psd, no context read" << endl;
    }

    //std::cerr << "Reading input parse tree... " << std::endl;
    if(!ReadParse(*m_parseStream, inputType))
    {
    	cerr << "Warning : If using soft syntax, no context read" << endl;
    }
    return inputType;
  } else {
    delete inputType;
    return NULL;
  }
}

//Damt hiero : read psd context file
int IOWrapper::ReadContext(std::istream& in, InputType* input)
{
    string line;
    if (getline(in, line, '\n').eof())
    return 0;

    //std::cerr << "READING CONTEXT : " << line << std::endl;

    vector<string> words = Tokenize(line, " ");
    for (size_t i = 0; i < words.size(); i++) {
    SetPSDContext(Tokenize(words[i], "|"),input);
    }
    return 1;
}

//Damt hiero : read input parse tree
int IOWrapper::ReadParse(std::istream& in, InputType* input)
{
    string line;
    if (getline(in, line, '\n').eof())
    return 0;

    //std::cerr << "READING PARSE : " << line << std::endl;

    //Read in parse tree
    size_t sourceSize = input->GetSize();
    input->m_parseTree->PopulateChart(sourceSize);
    input->m_parseTree->Read(line);

    //std::cerr << "Read in parse tree : "<< std::endl;
    //input->m_parseTree->Print(sourceSize);

    return 1;
}


void IOWrapper::SetPSDContext(const std::vector<std::string> &psdFact, InputType* input)
{
    //damt hiero debugging
    /*std::vector<std::string> :: const_iterator itr_fact;
    for(itr_fact = psdFact.begin(); itr_fact != psdFact.end(); itr_fact++)
    {
        std::cerr << "Added source factor : " << *itr_fact << std::endl;
    }*/
    input->m_PSDContext.push_back(psdFact);
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
      CHECK(factor);

      for (size_t i = 1 ; i < outputFactorOrder.size() ; i++) {
        const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[i]);
	CHECK(factor);

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
    CHECK(factor);

    cout << *factor << " ";
  }
}
/*
void OutputInput(std::vector<const Phrase*>& map, const ChartHypothesis* hypo)
{
	if (hypo->GetPrevHypos())
	{
	  	OutputInput(map, hypo->GetPrevHypos());
		map[hypo->GetCurrSourceWordsRange().GetStartPos()] = hypo->GetSourcePhrase();
	}
}

void OutputInput(std::ostream& os, const ChartHypothesis* hypo)
{
	size_t len = StaticData::Instance().GetInput()->GetSize();
	std::vector<const Phrase*> inp_phrases(len, 0);
	OutputInput(inp_phrases, hypo);
	for (size_t i=0; i<len; ++i)
		if (inp_phrases[i]) os << *inp_phrases[i];
}
*/

namespace {

typedef std::vector<std::pair<Word, WordsRange> > ApplicationContext;

// Given a hypothesis and sentence, reconstructs the 'application context' --
// the source RHS symbols of the SCFG rule that was applied, plus their spans.
void ReconstructApplicationContext(const ChartHypothesis &hypo,
                                   const Sentence &sentence,
                                   ApplicationContext &context)
{
  context.clear();
  const std::vector<const ChartHypothesis*> &prevHypos = hypo.GetPrevHypos();
  std::vector<const ChartHypothesis*>::const_iterator p = prevHypos.begin();
  std::vector<const ChartHypothesis*>::const_iterator end = prevHypos.end();
  const WordsRange &span = hypo.GetCurrSourceRange();
  size_t i = span.GetStartPos();
  while (i <= span.GetEndPos()) {
    if (p == end || i < (*p)->GetCurrSourceRange().GetStartPos()) {
      // Symbol is a terminal.
      const Word &symbol = sentence.GetWord(i);
      context.push_back(std::make_pair(symbol, WordsRange(i, i)));
      ++i;
    } else {
      // Symbol is a non-terminal.
      const Word &symbol = (*p)->GetTargetLHS();
      const WordsRange &range = (*p)->GetCurrSourceRange();
      context.push_back(std::make_pair(symbol, range));
      i = range.GetEndPos()+1;
      ++p;
    }
  }
}

// Emulates the old operator<<(ostream &, const DottedRule &) function.  The
// output format is a bit odd (reverse order and double spacing between symbols)
// but there are scripts and tools that expect the output of -T to look like
// that.
void WriteApplicationContext(std::ostream &out,
                             const ApplicationContext &context)
{
  assert(!context.empty());
  ApplicationContext::const_reverse_iterator p = context.rbegin();
  while (true) {
    out << p->second << "=" << p->first << " ";
    if (++p == context.rend()) {
      break;
    }
    out << " ";
  }
}

}  // anonymous namespace

void OutputTranslationOptions(std::ostream &out, std::ostream &alignOut, const ChartHypothesis *hypo, const Sentence &sentence, long translationId)
{
  static ApplicationContext applicationContext;
  typedef std::vector< const std::pair<size_t,size_t>* > AlignVec;
  // recursive
  if (hypo != NULL) {
    ReconstructApplicationContext(*hypo, sentence, applicationContext);
    out << "Trans Opt " << translationId
        << " " << hypo->GetCurrSourceRange()
        << ": ";
    WriteApplicationContext(out, applicationContext);
    out << ": " << hypo->GetCurrTargetPhrase().GetTargetLHS()
        << "->" << hypo->GetCurrTargetPhrase()
        << " " << hypo->GetTotalScore() << hypo->GetScoreBreakdown()
        << endl;
  }
  //damt_hiero: Printing out word alignments in the user-specified file
  AlignVec alignments = hypo->GetCurrTargetPhrase().GetWordAlignmentInfo().GetSortedAlignments();
  AlignVec::const_iterator it;
  for (it = alignments.begin(); it != alignments.end(); ++it)
  {
        const std::pair<size_t,size_t> &alignment = **it;
        //Word srcWord=hypo->GetCurrTargetPhrase().GetWord(alignment.first);
        //if (srcWord.IsNonTerminal()) continue;
        alignOut << alignment.first + hypo->GetCurrSourceRange().GetStartPos()-1 << "-" << alignment.second + hypo->GetCurrSourceRange().GetStartPos()-1 << "  ";
  }
  const std::vector<const ChartHypothesis*> &prevHypos = hypo->GetPrevHypos();
  std::vector<const ChartHypothesis*>::const_iterator iter;
  for (iter = prevHypos.begin(); iter != prevHypos.end(); ++iter) {
    const ChartHypothesis *prevHypo = *iter;
    OutputTranslationOptions(out, alignOut, prevHypo, sentence, translationId);
  }
}

void IOWrapper::OutputDetailedTranslationReport(
  const ChartHypothesis *hypo,
  const Sentence &sentence,
  long translationId)
{
  if (hypo == NULL) {
    return;
  }
  std::ostringstream out;
  std::ostringstream alignOut;
  OutputTranslationOptions(out, alignOut, hypo, sentence, translationId);
  CHECK(m_detailOutputCollector);
  m_detailOutputCollector->Write(translationId, out.str());
  m_wordAlignmentOutputCollector->Write(translationId, alignOut.str()+'\n');
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

    //Cell context features
    CellContextScoreProducer *ccsProducer = StaticData::Instance().GetCellContextScoreProducer();
    if (ccsProducer != NULL) {
      out << " " << ccsProducer->GetScoreProducerWeightShortName(0) << ":";
      vector<float> scores = path.GetScoreBreakdown().GetScoresForProducer(ccsProducer);
      for (size_t j = 0; j<scores.size(); ++j) {
        out << " " << scores[j];
      }
    }

    // total
    out << " |||" << path.GetTotalScore();

    /*
    if (includeAlignment) {
    	*m_nBestStream << " |||";
    	for (int currEdge = (int)edges.size() - 2 ; currEdge >= 0 ; currEdge--)
    	{
    		const ChartHypothesis &edge = *edges[currEdge];
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

    out << endl;
  }

  out <<std::flush;

  CHECK(m_nBestOutputCollector);
  m_nBestOutputCollector->Write(translationId, out.str());
}

void IOWrapper::FixPrecision(std::ostream &stream, size_t size)
{
  stream.setf(std::ios::fixed);
  stream.precision(size);
}

}

