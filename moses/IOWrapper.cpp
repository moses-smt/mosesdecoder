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
#include <stack>
#include <boost/algorithm/string.hpp>

#include "moses/Syntax/KBestExtractor.h"
#include "moses/Syntax/SHyperedge.h"
#include "moses/Syntax/S2T/DerivationWriter.h"
#include "moses/Syntax/PVertex.h"
#include "moses/Syntax/SVertex.h"

#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/Hypothesis.h"
#include "moses/WordsRange.h"
#include "moses/TrellisPathList.h"
#include "moses/StaticData.h"
#include "moses/FeatureVector.h"
#include "moses/InputFileStream.h"
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/StatelessFeatureFunction.h"
#include "moses/TreeInput.h"
#include "moses/ConfusionNet.h"
#include "moses/WordLattice.h"
#include "moses/Incremental.h"
#include "moses/ChartManager.h"


#include "util/exception.hh"

#include "IOWrapper.h"

using namespace std;

namespace Moses
{

IOWrapper::IOWrapper()
  :m_nBestStream(NULL)

  ,m_outputWordGraphStream(NULL)
  ,m_outputSearchGraphStream(NULL)
  ,m_detailedTranslationReportingStream(NULL)
  ,m_unknownsStream(NULL)
  ,m_alignmentInfoStream(NULL)
  ,m_latticeSamplesStream(NULL)

  ,m_singleBestOutputCollector(NULL)
  ,m_nBestOutputCollector(NULL)
  ,m_unknownsCollector(NULL)
  ,m_alignmentInfoCollector(NULL)
  ,m_searchGraphOutputCollector(NULL)
  ,m_detailedTranslationCollector(NULL)
  ,m_wordGraphCollector(NULL)
  ,m_latticeSamplesCollector(NULL)
  ,m_detailTreeFragmentsOutputCollector(NULL)

  ,m_surpressSingleBestOutput(false)

  ,spe_src(NULL)
  ,spe_trg(NULL)
  ,spe_aln(NULL)
{
  const StaticData &staticData = StaticData::Instance();

  m_inputFactorOrder = &staticData.GetInputFactorOrder();
  m_outputFactorOrder = &staticData.GetOutputFactorOrder();
  m_inputFactorUsed = FactorMask(*m_inputFactorOrder);

  size_t nBestSize = staticData.GetNBestSize();
  string nBestFilePath = staticData.GetNBestFilePath();

  staticData.GetParameter().SetParameter<string>(m_inputFilePath, "input-file", "");
  if (m_inputFilePath.empty()) {
	m_inputFile = NULL;
	m_inputStream = &cin;
  }
  else {
    VERBOSE(2,"IO from File" << endl);
    m_inputFile = new InputFileStream(m_inputFilePath);
    m_inputStream = m_inputFile;
  }

  if (nBestSize > 0) {
    if (nBestFilePath == "-" || nBestFilePath == "/dev/stdout") {
      m_nBestStream = &std::cout;
      m_nBestOutputCollector = new Moses::OutputCollector(&std::cout);
      m_surpressSingleBestOutput = true;
    } else {
      std::ofstream *file = new std::ofstream;
      file->open(nBestFilePath.c_str());
      m_nBestStream = file;

      m_nBestOutputCollector = new Moses::OutputCollector(file);
      //m_nBestOutputCollector->HoldOutputStream();
    }
  }

  // search graph output
  if (staticData.GetOutputSearchGraph()) {
    string fileName;
    if (staticData.GetOutputSearchGraphExtended()) {
    	staticData.GetParameter().SetParameter<string>(fileName, "output-search-graph-extended", "");
    }
    else {
    	staticData.GetParameter().SetParameter<string>(fileName, "output-search-graph", "");
    }
    std::ofstream *file = new std::ofstream;
    m_outputSearchGraphStream = file;
    file->open(fileName.c_str());
  }

  if (!staticData.GetOutputUnknownsFile().empty()) {
    m_unknownsStream = new std::ofstream(staticData.GetOutputUnknownsFile().c_str());
    m_unknownsCollector = new Moses::OutputCollector(m_unknownsStream);
    UTIL_THROW_IF2(!m_unknownsStream->good(),
                   "File for unknowns words could not be opened: " <<
                     staticData.GetOutputUnknownsFile());
  }

  if (!staticData.GetAlignmentOutputFile().empty()) {
    m_alignmentInfoStream = new std::ofstream(staticData.GetAlignmentOutputFile().c_str());
    m_alignmentInfoCollector = new Moses::OutputCollector(m_alignmentInfoStream);
    UTIL_THROW_IF2(!m_alignmentInfoStream->good(),
    		"File for alignment output could not be opened: " << staticData.GetAlignmentOutputFile());
  }

  if (staticData.GetOutputSearchGraph()) {
    string fileName;
	staticData.GetParameter().SetParameter<string>(fileName, "output-search-graph", "");

    std::ofstream *file = new std::ofstream;
    m_outputSearchGraphStream = file;
    file->open(fileName.c_str());
    m_searchGraphOutputCollector = new Moses::OutputCollector(m_outputSearchGraphStream);
  }

  // detailed translation reporting
  if (staticData.IsDetailedTranslationReportingEnabled()) {
    const std::string &path = staticData.GetDetailedTranslationReportingFilePath();
    m_detailedTranslationReportingStream = new std::ofstream(path.c_str());
    m_detailedTranslationCollector = new Moses::OutputCollector(m_detailedTranslationReportingStream);
  }

  if (staticData.IsDetailedTreeFragmentsTranslationReportingEnabled()) {
    const std::string &path = staticData.GetDetailedTreeFragmentsTranslationReportingFilePath();
    m_detailedTreeFragmentsTranslationReportingStream = new std::ofstream(path.c_str());
    m_detailTreeFragmentsOutputCollector = new Moses::OutputCollector(m_detailedTreeFragmentsTranslationReportingStream);
  }

  // wordgraph output
  if (staticData.GetOutputWordGraph()) {
    string fileName;
	staticData.GetParameter().SetParameter<string>(fileName, "output-word-graph", "");

    std::ofstream *file = new std::ofstream;
    m_outputWordGraphStream  = file;
    file->open(fileName.c_str());
    m_wordGraphCollector = new OutputCollector(m_outputWordGraphStream);
  }

  size_t latticeSamplesSize = staticData.GetLatticeSamplesSize();
  string latticeSamplesFile = staticData.GetLatticeSamplesFilePath();
  if (latticeSamplesSize) {
    if (latticeSamplesFile == "-" || latticeSamplesFile == "/dev/stdout") {
      m_latticeSamplesCollector = new OutputCollector();
      m_surpressSingleBestOutput = true;
    } else {
      m_latticeSamplesStream = new ofstream(latticeSamplesFile.c_str());
      if (!m_latticeSamplesStream->good()) {
        TRACE_ERR("ERROR: Failed to open " << latticeSamplesFile << " for lattice samples" << endl);
        exit(1);
      }
      m_latticeSamplesCollector = new OutputCollector(m_latticeSamplesStream);
    }
  }

  if (!m_surpressSingleBestOutput) {
    m_singleBestOutputCollector = new Moses::OutputCollector(&std::cout);
  }

  if (staticData.GetParameter().GetParam("spe-src")) {
	spe_src = new ifstream(staticData.GetParameter().GetParam("spe-src")->at(0).c_str());
    spe_trg = new ifstream(staticData.GetParameter().GetParam("spe-trg")->at(0).c_str());
    spe_aln = new ifstream(staticData.GetParameter().GetParam("spe-aln")->at(0).c_str());
  }
}

IOWrapper::~IOWrapper()
{
  if (m_inputFile != NULL)
    delete m_inputFile;
  if (m_nBestStream != NULL && !m_surpressSingleBestOutput) {
    // outputting n-best to file, rather than stdout. need to close file and delete obj
    delete m_nBestStream;
  }

  delete m_detailedTranslationReportingStream;
  delete m_alignmentInfoStream;
  delete m_unknownsStream;
  delete m_outputSearchGraphStream;
  delete m_outputWordGraphStream;
  delete m_latticeSamplesStream;

  delete m_singleBestOutputCollector;
  delete m_nBestOutputCollector;
  delete m_alignmentInfoCollector;
  delete m_searchGraphOutputCollector;
  delete m_detailedTranslationCollector;
  delete m_wordGraphCollector;
  delete m_latticeSamplesCollector;
  delete m_detailTreeFragmentsOutputCollector;

}

InputType*
IOWrapper::
GetInput(InputType* inputType)
{
  if(inputType->Read(*m_inputStream, *m_inputFactorOrder)) {
    return inputType;
  } else {
    delete inputType;
    return NULL;
  }
}

void IOWrapper::OutputTranslationOptions(std::ostream &out, ApplicationContext &applicationContext, const ChartHypothesis *hypo, const Sentence &sentence, long translationId)
{
  if (hypo != NULL) {
    OutputTranslationOption(out, applicationContext, hypo, sentence, translationId);
    out << std::endl;
  }

  // recursive
  const std::vector<const ChartHypothesis*> &prevHypos = hypo->GetPrevHypos();
  std::vector<const ChartHypothesis*>::const_iterator iter;
  for (iter = prevHypos.begin(); iter != prevHypos.end(); ++iter) {
    const ChartHypothesis *prevHypo = *iter;
    OutputTranslationOptions(out, applicationContext, prevHypo, sentence, translationId);
  }
}


void IOWrapper::OutputTranslationOptions(std::ostream &out, ApplicationContext &applicationContext, const search::Applied *applied, const Sentence &sentence, long translationId)
{
  if (applied != NULL) {
    OutputTranslationOption(out, applicationContext, applied, sentence, translationId);
    out << std::endl;
  }

  // recursive
  const search::Applied *child = applied->Children();
  for (size_t i = 0; i < applied->GetArity(); i++) {
      OutputTranslationOptions(out, applicationContext, child++, sentence, translationId);
  }
}

void IOWrapper::OutputTranslationOption(std::ostream &out, ApplicationContext &applicationContext, const ChartHypothesis *hypo, const Sentence &sentence, long translationId)
{
  ReconstructApplicationContext(*hypo, sentence, applicationContext);
  out << "Trans Opt " << translationId
      << " " << hypo->GetCurrSourceRange()
      << ": ";
  WriteApplicationContext(out, applicationContext);
  out << ": " << hypo->GetCurrTargetPhrase().GetTargetLHS()
      << "->" << hypo->GetCurrTargetPhrase()
      << " " << hypo->GetTotalScore() << hypo->GetScoreBreakdown();
}

void IOWrapper::OutputTranslationOption(std::ostream &out, ApplicationContext &applicationContext, const search::Applied *applied, const Sentence &sentence, long translationId)
{
  ReconstructApplicationContext(applied, sentence, applicationContext);
  const TargetPhrase &phrase = *static_cast<const TargetPhrase*>(applied->GetNote().vp);
  out << "Trans Opt " << translationId
      << " " << applied->GetRange()
      << ": ";
  WriteApplicationContext(out, applicationContext);
  out << ": " << phrase.GetTargetLHS()
      << "->" << phrase
      << " " << applied->GetScore(); // << hypo->GetScoreBreakdown() TODO: missing in incremental search hypothesis
}

// Given a hypothesis and sentence, reconstructs the 'application context' --
// the source RHS symbols of the SCFG rule that was applied, plus their spans.
void IOWrapper::ReconstructApplicationContext(const ChartHypothesis &hypo,
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

// Given a hypothesis and sentence, reconstructs the 'application context' --
// the source RHS symbols of the SCFG rule that was applied, plus their spans.
void IOWrapper::ReconstructApplicationContext(const search::Applied *applied,
    const Sentence &sentence,
    ApplicationContext &context)
{
  context.clear();
  const WordsRange &span = applied->GetRange();
  const search::Applied *child = applied->Children();
  size_t i = span.GetStartPos();
  size_t j = 0;

  while (i <= span.GetEndPos()) {
    if (j == applied->GetArity() || i < child->GetRange().GetStartPos()) {
      // Symbol is a terminal.
      const Word &symbol = sentence.GetWord(i);
      context.push_back(std::make_pair(symbol, WordsRange(i, i)));
      ++i;
    } else {
      // Symbol is a non-terminal.
      const Word &symbol = static_cast<const TargetPhrase*>(child->GetNote().vp)->GetTargetLHS();
      const WordsRange &range = child->GetRange();
      context.push_back(std::make_pair(symbol, range));
      i = range.GetEndPos()+1;
      ++child;
      ++j;
    }
  }
}

// Emulates the old operator<<(ostream &, const DottedRule &) function.  The
// output format is a bit odd (reverse order and double spacing between symbols)
// but there are scripts and tools that expect the output of -T to look like
// that.
void IOWrapper::WriteApplicationContext(std::ostream &out,
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

void IOWrapper::Backtrack(const Hypothesis *hypo)
{

  if (hypo->GetPrevHypo() != NULL) {
    VERBOSE(3,hypo->GetId() << " <= ");
    Backtrack(hypo->GetPrevHypo());
  }
}

bool IOWrapper::ReadInput(InputTypeEnum inputType, InputType*& source)
{
  delete source;
  switch(inputType) {
  case SentenceInput:
    source = GetInput(new Sentence);
    break;
  case ConfusionNetworkInput:
    source = GetInput(new ConfusionNet);
    break;
  case WordLatticeInput:
    source = GetInput(new WordLattice);
    break;
  case TreeInputType:
    source = GetInput(new TreeInput);
    break;
  default:
    TRACE_ERR("Unknown input type: " << inputType << "\n");
  }
  return (source ? true : false);
}

} // namespace

