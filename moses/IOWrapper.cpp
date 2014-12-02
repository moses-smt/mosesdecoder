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

std::map<size_t, const Factor*> IOWrapper::GetPlaceholders(const Hypothesis &hypo, FactorType placeholderFactor)
{
  const InputPath &inputPath = hypo.GetTranslationOption().GetInputPath();
  const Phrase &inputPhrase = inputPath.GetPhrase();

  std::map<size_t, const Factor*> ret;

  for (size_t sourcePos = 0; sourcePos < inputPhrase.GetSize(); ++sourcePos) {
    const Factor *factor = inputPhrase.GetFactor(sourcePos, placeholderFactor);
    if (factor) {
      std::set<size_t> targetPos = hypo.GetTranslationOption().GetTargetPhrase().GetAlignTerm().GetAlignmentsForSource(sourcePos);
      UTIL_THROW_IF2(targetPos.size() != 1,
    		  "Placeholder should be aligned to 1, and only 1, word");
      ret[*targetPos.begin()] = factor;
    }
  }

  return ret;
}


void IOWrapper::OutputBestHypo(const ChartHypothesis *hypo, long translationId)
{
  if (!m_singleBestOutputCollector)
    return;
  std::ostringstream out;
  FixPrecision(out);
  if (hypo != NULL) {
    VERBOSE(1,"BEST TRANSLATION: " << *hypo << endl);
    VERBOSE(3,"Best path: ");
    Backtrack(hypo);
    VERBOSE(3,"0" << std::endl);

    if (StaticData::Instance().GetOutputHypoScore()) {
      out << hypo->GetTotalScore() << " ";
    }

    if (StaticData::Instance().IsPathRecoveryEnabled()) {
      out << "||| ";
    }
    Phrase outPhrase(ARRAY_SIZE_INCR);
    hypo->GetOutputPhrase(outPhrase);

    // delete 1st & last
    UTIL_THROW_IF2(outPhrase.GetSize() < 2,
  		  "Output phrase should have contained at least 2 words (beginning and end-of-sentence)");

    outPhrase.RemoveWord(0);
    outPhrase.RemoveWord(outPhrase.GetSize() - 1);

    const std::vector<FactorType> outputFactorOrder = StaticData::Instance().GetOutputFactorOrder();
    string output = outPhrase.GetStringRep(outputFactorOrder);
    out << output << endl;
  } else {
    VERBOSE(1, "NO BEST TRANSLATION" << endl);

    if (StaticData::Instance().GetOutputHypoScore()) {
      out << "0 ";
    }

    out << endl;
  }
  m_singleBestOutputCollector->Write(translationId, out.str());
}

void IOWrapper::OutputBestHypo(search::Applied applied, long translationId)
{
  if (!m_singleBestOutputCollector) return;
  std::ostringstream out;
  FixPrecision(out);
  if (StaticData::Instance().GetOutputHypoScore()) {
    out << applied.GetScore() << ' ';
  }
  Phrase outPhrase;
  Incremental::ToPhrase(applied, outPhrase);
  // delete 1st & last
  UTIL_THROW_IF2(outPhrase.GetSize() < 2,
		  "Output phrase should have contained at least 2 words (beginning and end-of-sentence)");
  outPhrase.RemoveWord(0);
  outPhrase.RemoveWord(outPhrase.GetSize() - 1);
  out << outPhrase.GetStringRep(StaticData::Instance().GetOutputFactorOrder());
  out << '\n';
  m_singleBestOutputCollector->Write(translationId, out.str());

  VERBOSE(1,"BEST TRANSLATION: " << outPhrase << "[total=" << applied.GetScore() << "]" << endl);
}

void IOWrapper::OutputBestNone(long translationId)
{
  if (!m_singleBestOutputCollector) return;
  if (StaticData::Instance().GetOutputHypoScore()) {
    m_singleBestOutputCollector->Write(translationId, "0 \n");
  } else {
    m_singleBestOutputCollector->Write(translationId, "\n");
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

void IOWrapper::OutputDetailedTranslationReport(
  const search::Applied *applied,
  const Sentence &sentence,
  long translationId)
{
  if (applied == NULL) {
    return;
  }
  std::ostringstream out;
  ApplicationContext applicationContext;

  OutputTranslationOptions(out, applicationContext, applied, sentence, translationId);
  UTIL_THROW_IF2(m_detailedTranslationCollector == NULL,
                  "No ouput file for detailed reports specified");
  m_detailedTranslationCollector->Write(translationId, out.str());
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

void IOWrapper::OutputDetailedTreeFragmentsTranslationReport(
  const ChartHypothesis *hypo,
  const Sentence &sentence,
  long translationId)
{
  if (hypo == NULL) {
    return;
  }
  std::ostringstream out;
  ApplicationContext applicationContext;

  OutputTreeFragmentsTranslationOptions(out, applicationContext, hypo, sentence, translationId);
  UTIL_THROW_IF2(m_detailTreeFragmentsOutputCollector == NULL,
		  "No output file for tree fragments specified");

  //Tree of full sentence
  const StatefulFeatureFunction* treeStructure = StaticData::Instance().GetTreeStructure();
  if (treeStructure != NULL) {
    const vector<const StatefulFeatureFunction*>& sff = StatefulFeatureFunction::GetStatefulFeatureFunctions();
    for( size_t i=0; i<sff.size(); i++ ) {
        if (sff[i] == treeStructure) {
        const TreeState* tree = dynamic_cast<const TreeState*>(hypo->GetFFState(i));
        out << "Full Tree " << translationId << ": " << tree->GetTree()->GetString() << "\n";
        break;
        }
    }
  }

  m_detailTreeFragmentsOutputCollector->Write(translationId, out.str());

}

void IOWrapper::OutputDetailedTreeFragmentsTranslationReport(
  const search::Applied *applied,
  const Sentence &sentence,
  long translationId)
{
  if (applied == NULL) {
    return;
  }
  std::ostringstream out;
  ApplicationContext applicationContext;

  OutputTreeFragmentsTranslationOptions(out, applicationContext, applied, sentence, translationId);
  UTIL_THROW_IF2(m_detailTreeFragmentsOutputCollector == NULL,
                  "No output file for tree fragments specified");

  //Tree of full sentence
  //TODO: incremental search doesn't support stateful features

  m_detailTreeFragmentsOutputCollector->Write(translationId, out.str());

}

void IOWrapper::OutputTreeFragmentsTranslationOptions(std::ostream &out, ApplicationContext &applicationContext, const ChartHypothesis *hypo, const Sentence &sentence, long translationId)
{

  if (hypo != NULL) {
    OutputTranslationOption(out, applicationContext, hypo, sentence, translationId);

    const TargetPhrase &currTarPhr = hypo->GetCurrTargetPhrase();

    out << " ||| ";
    if (const PhraseProperty *property = currTarPhr.GetProperty("Tree")) {
      out << " " << *property->GetValueString();
    } else {
      out << " " << "noTreeInfo";
    }
    out << std::endl;
  }

  // recursive
  const std::vector<const ChartHypothesis*> &prevHypos = hypo->GetPrevHypos();
  std::vector<const ChartHypothesis*>::const_iterator iter;
  for (iter = prevHypos.begin(); iter != prevHypos.end(); ++iter) {
    const ChartHypothesis *prevHypo = *iter;
    OutputTreeFragmentsTranslationOptions(out, applicationContext, prevHypo, sentence, translationId);
  }
}

void IOWrapper::OutputTreeFragmentsTranslationOptions(std::ostream &out, ApplicationContext &applicationContext, const search::Applied *applied, const Sentence &sentence, long translationId)
{

  if (applied != NULL) {
    OutputTranslationOption(out, applicationContext, applied, sentence, translationId);

    const TargetPhrase &currTarPhr = *static_cast<const TargetPhrase*>(applied->GetNote().vp);

    out << " ||| ";
    if (const PhraseProperty *property = currTarPhr.GetProperty("Tree")) {
      out << " " << *property->GetValueString();
    } else {
      out << " " << "noTreeInfo";
    }
    out << std::endl;
  }

  // recursive
  const search::Applied *child = applied->Children();
  for (size_t i = 0; i < applied->GetArity(); i++) {
      OutputTreeFragmentsTranslationOptions(out, applicationContext, child++, sentence, translationId);
  }
}

/***
 * print surface factor only for the given phrase
 */
void IOWrapper::OutputSurface(std::ostream &out, const Phrase &phrase, const std::vector<FactorType> &outputFactorOrder, bool reportAllFactors)
{
  UTIL_THROW_IF2(outputFactorOrder.size() == 0,
		  "Cannot be empty phrase");
  if (reportAllFactors == true) {
    out << phrase;
  } else {
    size_t size = phrase.GetSize();
    for (size_t pos = 0 ; pos < size ; pos++) {
      const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[0]);
      out << *factor;
      UTIL_THROW_IF2(factor == NULL,
    		  "Empty factor 0 at position " << pos);

      for (size_t i = 1 ; i < outputFactorOrder.size() ; i++) {
        const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[i]);
        UTIL_THROW_IF2(factor == NULL,
      		  "Empty factor " << i << " at position " << pos);

        out << "|" << *factor;
      }
      out << " ";
    }
  }
}

void IOWrapper::OutputAlignment(size_t translationId , const Moses::ChartHypothesis *hypo)
{
  ostringstream out;

  if (hypo) {
    Alignments retAlign;
    OutputAlignment(retAlign, hypo, 0);

    // output alignments
    Alignments::const_iterator iter;
    for (iter = retAlign.begin(); iter != retAlign.end(); ++iter) {
      const pair<size_t, size_t> &alignPoint = *iter;
      out << alignPoint.first << "-" << alignPoint.second << " ";
    }
  }
  out << endl;

  m_alignmentInfoCollector->Write(translationId, out.str());
}

size_t IOWrapper::OutputAlignment(Alignments &retAlign, const Moses::ChartHypothesis *hypo, size_t startTarget)
{
  size_t totalTargetSize = 0;
  size_t startSource = hypo->GetCurrSourceRange().GetStartPos();

  const TargetPhrase &tp = hypo->GetCurrTargetPhrase();

  size_t thisSourceSize = CalcSourceSize(hypo);

  // position of each terminal word in translation rule, irrespective of alignment
  // if non-term, number is undefined
  vector<size_t> sourceOffsets(thisSourceSize, 0);
  vector<size_t> targetOffsets(tp.GetSize(), 0);

  const vector<const ChartHypothesis*> &prevHypos = hypo->GetPrevHypos();

  const AlignmentInfo &aiNonTerm = hypo->GetCurrTargetPhrase().GetAlignNonTerm();
  vector<size_t> sourceInd2pos = aiNonTerm.GetSourceIndex2PosMap();
  const AlignmentInfo::NonTermIndexMap &targetPos2SourceInd = aiNonTerm.GetNonTermIndexMap();

  UTIL_THROW_IF2(sourceInd2pos.size() != prevHypos.size(), "Error");

  size_t targetInd = 0;
  for (size_t targetPos = 0; targetPos < tp.GetSize(); ++targetPos) {
    if (tp.GetWord(targetPos).IsNonTerminal()) {
  	  UTIL_THROW_IF2(targetPos >= targetPos2SourceInd.size(), "Error");
      size_t sourceInd = targetPos2SourceInd[targetPos];
      size_t sourcePos = sourceInd2pos[sourceInd];

      const ChartHypothesis *prevHypo = prevHypos[sourceInd];

      // calc source size
      size_t sourceSize = prevHypo->GetCurrSourceRange().GetNumWordsCovered();
      sourceOffsets[sourcePos] = sourceSize;

      // calc target size.
      // Recursively look thru child hypos
      size_t currStartTarget = startTarget + totalTargetSize;
      size_t targetSize = OutputAlignment(retAlign, prevHypo, currStartTarget);
      targetOffsets[targetPos] = targetSize;

      totalTargetSize += targetSize;
      ++targetInd;
    } else {
      ++totalTargetSize;
    }
  }

  // convert position within translation rule to absolute position within
  // source sentence / output sentence
  ShiftOffsets(sourceOffsets, startSource);
  ShiftOffsets(targetOffsets, startTarget);

  // get alignments from this hypo
  const AlignmentInfo &aiTerm = hypo->GetCurrTargetPhrase().GetAlignTerm();

  // add to output arg, offsetting by source & target
  AlignmentInfo::const_iterator iter;
  for (iter = aiTerm.begin(); iter != aiTerm.end(); ++iter) {
    const std::pair<size_t,size_t> &align = *iter;
    size_t relSource = align.first;
    size_t relTarget = align.second;
    size_t absSource = sourceOffsets[relSource];
    size_t absTarget = targetOffsets[relTarget];

    pair<size_t, size_t> alignPoint(absSource, absTarget);
    pair<Alignments::iterator, bool> ret = retAlign.insert(alignPoint);
    UTIL_THROW_IF2(!ret.second, "Error");

  }

  return totalTargetSize;
}

size_t IOWrapper::CalcSourceSize(const Moses::ChartHypothesis *hypo)
{
  size_t ret = hypo->GetCurrSourceRange().GetNumWordsCovered();
  const std::vector<const ChartHypothesis*> &prevHypos = hypo->GetPrevHypos();
  for (size_t i = 0; i < prevHypos.size(); ++i) {
    size_t childSize = prevHypos[i]->GetCurrSourceRange().GetNumWordsCovered();
    ret -= (childSize - 1);
  }
  return ret;
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
  ApplicationContext applicationContext;

  OutputTranslationOptions(out, applicationContext, hypo, sentence, translationId);
  UTIL_THROW_IF2(m_detailedTranslationCollector == NULL,
		  "No ouput file for detailed reports specified");
  m_detailedTranslationCollector->Write(translationId, out.str());
}

//DIMw
void IOWrapper::OutputDetailedAllTranslationReport(
  const std::vector<boost::shared_ptr<Moses::ChartKBestExtractor::Derivation> > &nBestList,
  const ChartManager &manager,
  const Sentence &sentence,
  long translationId)
{
  std::ostringstream out;
  ApplicationContext applicationContext;

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
        OutputTranslationOptions(out, applicationContext, *iter,
                                 sentence, translationId);
      }
    }
  }
  UTIL_THROW_IF2(m_detailedTranslationCollector == NULL,
		  "No output file for details specified");
  m_detailedTranslationCollector->Write(translationId, out.str());
}

void IOWrapper::OutputUnknowns(const std::vector<Moses::Phrase*> &unknowns,
                               long translationId)
{
  std::ostringstream out;
  for (std::size_t i = 0; i < unknowns.size(); ++i) {
    out << *(unknowns[i]);
  }
  out << std::endl;
  m_unknownsCollector->Write(translationId, out.str());
}

void IOWrapper::OutputNBestList(const ChartKBestExtractor::KBestVec &nBestList,
                                long translationId)
{
  std::ostringstream out;

  if (m_nBestOutputCollector->OutputIsCout()) {
    // Set precision only if we're writing the n-best list to cout.  This is to
    // preserve existing behaviour, but should probably be done either way.
    FixPrecision(out);
  }

  bool includeWordAlignment =
      StaticData::Instance().PrintAlignmentInfoInNbest();

  bool PrintNBestTrees = StaticData::Instance().PrintNBestTrees();

  for (ChartKBestExtractor::KBestVec::const_iterator p = nBestList.begin();
       p != nBestList.end(); ++p) {
    const ChartKBestExtractor::Derivation &derivation = **p;

    // get the derivation's target-side yield
    Phrase outputPhrase = ChartKBestExtractor::GetOutputPhrase(derivation);

    // delete <s> and </s>
    UTIL_THROW_IF2(outputPhrase.GetSize() < 2,
        "Output phrase should have contained at least 2 words (beginning and end-of-sentence)");
    outputPhrase.RemoveWord(0);
    outputPhrase.RemoveWord(outputPhrase.GetSize() - 1);

    // print the translation ID, surface factors, and scores
    out << translationId << " ||| ";
    OutputSurface(out, outputPhrase, *m_outputFactorOrder, false);
    out << " ||| ";
    OutputAllFeatureScores(derivation.scoreBreakdown, out);
    out << " ||| " << derivation.score;

    // optionally, print word alignments
    if (includeWordAlignment) {
      out << " ||| ";
      Alignments align;
      OutputAlignmentNBest(align, derivation, 0);
      for (Alignments::const_iterator q = align.begin(); q != align.end();
           ++q) {
        out << q->first << "-" << q->second << " ";
      }
    }

    // optionally, print tree
    if (PrintNBestTrees) {
      TreePointer tree = ChartKBestExtractor::GetOutputTree(derivation);
      out << " ||| " << tree->GetString();
    }

    out << std::endl;
  }

  assert(m_nBestOutputCollector);
  m_nBestOutputCollector->Write(translationId, out.str());
}

size_t IOWrapper::OutputAlignmentNBest(
    Alignments &retAlign,
    const Moses::ChartKBestExtractor::Derivation &derivation,
    size_t startTarget)
{
  const ChartHypothesis &hypo = derivation.edge.head->hypothesis;

  size_t totalTargetSize = 0;
  size_t startSource = hypo.GetCurrSourceRange().GetStartPos();

  const TargetPhrase &tp = hypo.GetCurrTargetPhrase();

  size_t thisSourceSize = CalcSourceSize(&hypo);

  // position of each terminal word in translation rule, irrespective of alignment
  // if non-term, number is undefined
  vector<size_t> sourceOffsets(thisSourceSize, 0);
  vector<size_t> targetOffsets(tp.GetSize(), 0);

  const AlignmentInfo &aiNonTerm = hypo.GetCurrTargetPhrase().GetAlignNonTerm();
  vector<size_t> sourceInd2pos = aiNonTerm.GetSourceIndex2PosMap();
  const AlignmentInfo::NonTermIndexMap &targetPos2SourceInd = aiNonTerm.GetNonTermIndexMap();

  UTIL_THROW_IF2(sourceInd2pos.size() != derivation.subderivations.size(),
                 "Error");

  size_t targetInd = 0;
  for (size_t targetPos = 0; targetPos < tp.GetSize(); ++targetPos) {
    if (tp.GetWord(targetPos).IsNonTerminal()) {
      UTIL_THROW_IF2(targetPos >= targetPos2SourceInd.size(), "Error");
      size_t sourceInd = targetPos2SourceInd[targetPos];
      size_t sourcePos = sourceInd2pos[sourceInd];

      const Moses::ChartKBestExtractor::Derivation &subderivation =
        *derivation.subderivations[sourceInd];

      // calc source size
      size_t sourceSize = subderivation.edge.head->hypothesis.GetCurrSourceRange().GetNumWordsCovered();
      sourceOffsets[sourcePos] = sourceSize;

      // calc target size.
      // Recursively look thru child hypos
      size_t currStartTarget = startTarget + totalTargetSize;
      size_t targetSize = OutputAlignmentNBest(retAlign, subderivation,
                                               currStartTarget);
      targetOffsets[targetPos] = targetSize;

      totalTargetSize += targetSize;
      ++targetInd;
    } else {
      ++totalTargetSize;
    }
  }

  // convert position within translation rule to absolute position within
  // source sentence / output sentence
  ShiftOffsets(sourceOffsets, startSource);
  ShiftOffsets(targetOffsets, startTarget);

  // get alignments from this hypo
  const AlignmentInfo &aiTerm = hypo.GetCurrTargetPhrase().GetAlignTerm();

  // add to output arg, offsetting by source & target
  AlignmentInfo::const_iterator iter;
  for (iter = aiTerm.begin(); iter != aiTerm.end(); ++iter) {
    const std::pair<size_t,size_t> &align = *iter;
    size_t relSource = align.first;
    size_t relTarget = align.second;
    size_t absSource = sourceOffsets[relSource];
    size_t absTarget = targetOffsets[relTarget];

    pair<size_t, size_t> alignPoint(absSource, absTarget);
    pair<Alignments::iterator, bool> ret = retAlign.insert(alignPoint);
    UTIL_THROW_IF2(!ret.second, "Error");
  }

  return totalTargetSize;
}

//////////////////////////////////////////////////////////////////////////
/***
 * print surface factor only for the given phrase
 */
void IOWrapper::OutputSurface(std::ostream &out, const Hypothesis &edge, const std::vector<FactorType> &outputFactorOrder,
                   char reportSegmentation, bool reportAllFactors)
{
  UTIL_THROW_IF2(outputFactorOrder.size() == 0,
		  "Must specific at least 1 output factor");
  const TargetPhrase& phrase = edge.GetCurrTargetPhrase();
  bool markUnknown = StaticData::Instance().GetMarkUnknown();
  if (reportAllFactors == true) {
    out << phrase;
  } else {
    FactorType placeholderFactor = StaticData::Instance().GetPlaceholderFactor();

    std::map<size_t, const Factor*> placeholders;
    if (placeholderFactor != NOT_FOUND) {
      // creates map of target position -> factor for placeholders
      placeholders = GetPlaceholders(edge, placeholderFactor);
    }

    size_t size = phrase.GetSize();
    for (size_t pos = 0 ; pos < size ; pos++) {
      const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[0]);

      if (placeholders.size()) {
        // do placeholders
        std::map<size_t, const Factor*>::const_iterator iter = placeholders.find(pos);
        if (iter != placeholders.end()) {
          factor = iter->second;
        }
      }

      UTIL_THROW_IF2(factor == NULL,
    		  "No factor 0 at position " << pos);

      //preface surface form with UNK if marking unknowns
      const Word &word = phrase.GetWord(pos);
      if(markUnknown && word.IsOOV()) {
        out << "UNK" << *factor;
      } else {
        out << *factor;
      }

      for (size_t i = 1 ; i < outputFactorOrder.size() ; i++) {
        const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[i]);
        UTIL_THROW_IF2(factor == NULL,
      		  "No factor " << i << " at position " << pos);

        out << "|" << *factor;
      }
      out << " ";
    }
  }

  // trace ("report segmentation") option "-t" / "-tt"
  if (reportSegmentation > 0 && phrase.GetSize() > 0) {
    const WordsRange &sourceRange = edge.GetCurrSourceWordsRange();
    const int sourceStart = sourceRange.GetStartPos();
    const int sourceEnd = sourceRange.GetEndPos();
    out << "|" << sourceStart << "-" << sourceEnd;    // enriched "-tt"
    if (reportSegmentation == 2) {
      out << ",wa=";
      const AlignmentInfo &ai = edge.GetCurrTargetPhrase().GetAlignTerm();
      OutputAlignment(out, ai, 0, 0);
      out << ",total=";
      out << edge.GetScore() - edge.GetPrevHypo()->GetScore();
      out << ",";
      ScoreComponentCollection scoreBreakdown(edge.GetScoreBreakdown());
      scoreBreakdown.MinusEquals(edge.GetPrevHypo()->GetScoreBreakdown());
      OutputAllFeatureScores(scoreBreakdown, out);
    }
    out << "| ";
  }
}

void IOWrapper::OutputBestSurface(std::ostream &out, const Hypothesis *hypo, const std::vector<FactorType> &outputFactorOrder,
                       char reportSegmentation, bool reportAllFactors)
{
  if (hypo != NULL) {
    // recursively retrace this best path through the lattice, starting from the end of the hypothesis sentence
    OutputBestSurface(out, hypo->GetPrevHypo(), outputFactorOrder, reportSegmentation, reportAllFactors);
    OutputSurface(out, *hypo, outputFactorOrder, reportSegmentation, reportAllFactors);
  }
}

void IOWrapper::OutputAlignment(ostream &out, const AlignmentInfo &ai, size_t sourceOffset, size_t targetOffset)
{
  typedef std::vector< const std::pair<size_t,size_t>* > AlignVec;
  AlignVec alignments = ai.GetSortedAlignments();

  AlignVec::const_iterator it;
  for (it = alignments.begin(); it != alignments.end(); ++it) {
    const std::pair<size_t,size_t> &alignment = **it;
    out << alignment.first + sourceOffset << "-" << alignment.second + targetOffset << " ";
  }

}

void IOWrapper::OutputAlignment(ostream &out, const vector<const Hypothesis *> &edges)
{
  size_t targetOffset = 0;

  for (int currEdge = (int)edges.size() - 1 ; currEdge >= 0 ; currEdge--) {
    const Hypothesis &edge = *edges[currEdge];
    const TargetPhrase &tp = edge.GetCurrTargetPhrase();
    size_t sourceOffset = edge.GetCurrSourceWordsRange().GetStartPos();

    OutputAlignment(out, tp.GetAlignTerm(), sourceOffset, targetOffset);

    targetOffset += tp.GetSize();
  }
  // Removing std::endl here breaks -alignment-output-file, so stop doing that, please :)
  // Or fix it somewhere else.
  out << std::endl;
}

void IOWrapper::OutputAlignment(std::ostream &out, const Moses::Hypothesis *hypo)
{
  std::vector<const Hypothesis *> edges;
  const Hypothesis *currentHypo = hypo;
  while (currentHypo) {
    edges.push_back(currentHypo);
    currentHypo = currentHypo->GetPrevHypo();
  }

  OutputAlignment(out, edges);

}

void IOWrapper::OutputAlignment(OutputCollector* collector, size_t lineNo , const vector<const Hypothesis *> &edges)
{
  ostringstream out;
  OutputAlignment(out, edges);

  collector->Write(lineNo,out.str());
}

void IOWrapper::OutputAlignment(OutputCollector* collector, size_t lineNo , const Hypothesis *hypo)
{
  if (collector) {
    std::vector<const Hypothesis *> edges;
    const Hypothesis *currentHypo = hypo;
    while (currentHypo) {
      edges.push_back(currentHypo);
      currentHypo = currentHypo->GetPrevHypo();
    }

    OutputAlignment(collector,lineNo, edges);
  }
}

void IOWrapper::OutputAlignment(OutputCollector* collector, size_t lineNo , const TrellisPath &path)
{
  if (collector) {
    OutputAlignment(collector,lineNo, path.GetEdges());
  }
}

void IOWrapper::OutputBestHypo(const Moses::TrellisPath &path, long /*translationId*/, char reportSegmentation, bool reportAllFactors, std::ostream &out)
{
  const std::vector<const Hypothesis *> &edges = path.GetEdges();

  for (int currEdge = (int)edges.size() - 1 ; currEdge >= 0 ; currEdge--) {
    const Hypothesis &edge = *edges[currEdge];
    OutputSurface(out, edge, StaticData::Instance().GetOutputFactorOrder(), reportSegmentation, reportAllFactors);
  }
  out << endl;
}

void IOWrapper::Backtrack(const Hypothesis *hypo)
{

  if (hypo->GetPrevHypo() != NULL) {
    VERBOSE(3,hypo->GetId() << " <= ");
    Backtrack(hypo->GetPrevHypo());
  }
}

void IOWrapper::OutputBestHypo(const std::vector<Word>&  mbrBestHypo, long /*translationId*/, char /*reportSegmentation*/, bool /*reportAllFactors*/, ostream& out)
{

  for (size_t i = 0 ; i < mbrBestHypo.size() ; i++) {
    const Factor *factor = mbrBestHypo[i].GetFactor(StaticData::Instance().GetOutputFactorOrder()[0]);
    UTIL_THROW_IF2(factor == NULL,
  		  "No factor 0 at position " << i);
    if (i>0) out << " " << *factor;
    else     out << *factor;
  }
  out << endl;
}


void IOWrapper::OutputInput(std::vector<const Phrase*>& map, const Hypothesis* hypo)
{
  if (hypo->GetPrevHypo()) {
    OutputInput(map, hypo->GetPrevHypo());
    map[hypo->GetCurrSourceWordsRange().GetStartPos()] = &hypo->GetTranslationOption().GetInputPath().GetPhrase();
  }
}

void IOWrapper::OutputInput(std::ostream& os, const Hypothesis* hypo)
{
  size_t len = hypo->GetInput().GetSize();
  std::vector<const Phrase*> inp_phrases(len, 0);
  OutputInput(inp_phrases, hypo);
  for (size_t i=0; i<len; ++i)
    if (inp_phrases[i]) os << *inp_phrases[i];
}

void IOWrapper::OutputBestHypo(const Hypothesis *hypo, long /*translationId*/, char reportSegmentation, bool reportAllFactors)
{
  if (hypo != NULL) {
    VERBOSE(1,"BEST TRANSLATION: " << *hypo << endl);
    VERBOSE(3,"Best path: ");
    Backtrack(hypo);
    VERBOSE(3,"0" << std::endl);
    if (!m_surpressSingleBestOutput) {
      if (StaticData::Instance().GetOutputHypoScore()) {
        cout << hypo->GetTotalScore() << " ";
      }

      if (StaticData::Instance().IsPathRecoveryEnabled()) {
        OutputInput(cout, hypo);
        cout << "||| ";
      }
      OutputBestSurface(cout, hypo, *m_outputFactorOrder, reportSegmentation, reportAllFactors);
      cout << endl;
    }
  } else {
    VERBOSE(1, "NO BEST TRANSLATION" << endl);
    if (!m_surpressSingleBestOutput) {
      cout << endl;
    }
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

void IOWrapper::OutputAllFeatureScores(const Moses::ScoreComponentCollection &features
                            , std::ostream &out)
{
  std::string lastName = "";
  const vector<const StatefulFeatureFunction*>& sff = StatefulFeatureFunction::GetStatefulFeatureFunctions();
  for( size_t i=0; i<sff.size(); i++ ) {
    const StatefulFeatureFunction *ff = sff[i];
    if (ff->GetScoreProducerDescription() != "BleuScoreFeature"
        && ff->IsTuneable()) {
      OutputFeatureScores( out, features, ff, lastName );
    }
  }
  const vector<const StatelessFeatureFunction*>& slf = StatelessFeatureFunction::GetStatelessFeatureFunctions();
  for( size_t i=0; i<slf.size(); i++ ) {
    const StatelessFeatureFunction *ff = slf[i];
    if (ff->IsTuneable()) {
      OutputFeatureScores( out, features, ff, lastName );
    }
  }
}

void IOWrapper::OutputFeatureScores( std::ostream& out
                          , const ScoreComponentCollection &features
                          , const FeatureFunction *ff
                          , std::string &lastName )
{
  const StaticData &staticData = StaticData::Instance();
  bool labeledOutput = staticData.IsLabeledNBestList();

  // regular features (not sparse)
  if (ff->GetNumScoreComponents() != 0) {
    if( labeledOutput && lastName != ff->GetScoreProducerDescription() ) {
      lastName = ff->GetScoreProducerDescription();
      out << " " << lastName << "=";
    }
    vector<float> scores = features.GetScoresForProducer( ff );
    for (size_t j = 0; j<scores.size(); ++j) {
      out << " " << scores[j];
    }
  }

  // sparse features
  const FVector scores = features.GetVectorForProducer( ff );
  for(FVector::FNVmap::const_iterator i = scores.cbegin(); i != scores.cend(); i++) {
    out << " " << i->first << "= " << i->second;
  }
}

void IOWrapper::OutputLatticeMBRNBest(std::ostream& out, const vector<LatticeMBRSolution>& solutions,long translationId)
{
  for (vector<LatticeMBRSolution>::const_iterator si = solutions.begin(); si != solutions.end(); ++si) {
    out << translationId;
    out << " |||";
    const vector<Word> mbrHypo = si->GetWords();
    for (size_t i = 0 ; i < mbrHypo.size() ; i++) {
      const Factor *factor = mbrHypo[i].GetFactor(StaticData::Instance().GetOutputFactorOrder()[0]);
      if (i>0) out << " " << *factor;
      else     out << *factor;
    }
    out << " |||";
    out << " map: " << si->GetMapScore();
    out << " w: " << mbrHypo.size();
    const vector<float>& ngramScores = si->GetNgramScores();
    for (size_t i = 0; i < ngramScores.size(); ++i) {
      out << " " << ngramScores[i];
    }
    out << " ||| " << si->GetScore();

    out << endl;
  }
}


void IOWrapper::OutputLatticeMBRNBestList(const vector<LatticeMBRSolution>& solutions,long translationId)
{
  OutputLatticeMBRNBest(*m_nBestStream, solutions,translationId);
}

////////////////////////////
#include "moses/Syntax/PVertex.h"
#include "moses/Syntax/S2T/DerivationWriter.h"

void IOWrapper::OutputDetailedTranslationReport(const Syntax::SHyperedge *best,
                                                long translationId)
{
  if (best == NULL) {
    return;
  }
  std::ostringstream out;
  Syntax::S2T::DerivationWriter::Write(*best, translationId, out);
  UTIL_THROW_IF2(m_detailedTranslationCollector == NULL,
		  "No ouput file for detailed reports specified");
  m_detailedTranslationCollector->Write(translationId, out.str());
}

void IOWrapper::OutputBestHypo(const Syntax::SHyperedge *best,
                               long translationId)
{
  if (!m_singleBestOutputCollector) {
    return;
  }
  std::ostringstream out;
  FixPrecision(out);
  if (best == NULL) {
    VERBOSE(1, "NO BEST TRANSLATION" << std::endl);
    if (StaticData::Instance().GetOutputHypoScore()) {
      out << "0 ";
    }
  } else {
    if (StaticData::Instance().GetOutputHypoScore()) {
      out << best->score << " ";
    }
    Phrase yield = Syntax::GetOneBestTargetYield(*best);
    // delete 1st & last
    UTIL_THROW_IF2(yield.GetSize() < 2,
        "Output phrase should have contained at least 2 words (beginning and end-of-sentence)");
    yield.RemoveWord(0);
    yield.RemoveWord(yield.GetSize()-1);
    out << yield.GetStringRep(StaticData::Instance().GetOutputFactorOrder());
    out << '\n';
  }
  m_singleBestOutputCollector->Write(translationId, out.str());
}

void IOWrapper::OutputNBestList(
    const Syntax::KBestExtractor::KBestVec &nBestList, long translationId)
{
  std::ostringstream out;

  if (m_nBestOutputCollector->OutputIsCout()) {
    // Set precision only if we're writing the n-best list to cout.  This is to
    // preserve existing behaviour, but should probably be done either way.
    FixPrecision(out);
  }

  bool includeWordAlignment =
      StaticData::Instance().PrintAlignmentInfoInNbest();

  bool PrintNBestTrees = StaticData::Instance().PrintNBestTrees();

  for (Syntax::KBestExtractor::KBestVec::const_iterator p = nBestList.begin();
       p != nBestList.end(); ++p) {
    const Syntax::KBestExtractor::Derivation &derivation = **p;

    // get the derivation's target-side yield
    Phrase outputPhrase = Syntax::KBestExtractor::GetOutputPhrase(derivation);

    // delete <s> and </s>
    UTIL_THROW_IF2(outputPhrase.GetSize() < 2,
        "Output phrase should have contained at least 2 words (beginning and end-of-sentence)");
    outputPhrase.RemoveWord(0);
    outputPhrase.RemoveWord(outputPhrase.GetSize() - 1);

    // print the translation ID, surface factors, and scores
    out << translationId << " ||| ";
    OutputSurface(out, outputPhrase, *m_outputFactorOrder, false);
    out << " ||| ";
    OutputAllFeatureScores(derivation.scoreBreakdown, out);
    out << " ||| " << derivation.score;

    // optionally, print word alignments
    if (includeWordAlignment) {
      out << " ||| ";
      Alignments align;
      OutputAlignmentNBest(align, derivation, 0);
      for (Alignments::const_iterator q = align.begin(); q != align.end();
           ++q) {
        out << q->first << "-" << q->second << " ";
      }
    }

    // optionally, print tree
    if (PrintNBestTrees) {
      TreePointer tree = Syntax::KBestExtractor::GetOutputTree(derivation);
      out << " ||| " << tree->GetString();
    }

    out << std::endl;
  }

  assert(m_nBestOutputCollector);
  m_nBestOutputCollector->Write(translationId, out.str());
}

size_t IOWrapper::CalcSourceSize(const Syntax::KBestExtractor::Derivation &d) const
{
  using namespace Moses::Syntax;

  const Syntax::SHyperedge &shyperedge = d.edge->shyperedge;
  size_t ret = shyperedge.head->pvertex->span.GetNumWordsCovered();
  for (size_t i = 0; i < shyperedge.tail.size(); ++i) {
    size_t childSize = shyperedge.tail[i]->pvertex->span.GetNumWordsCovered();
    ret -= (childSize - 1);
  }
  return ret;
}

size_t IOWrapper::OutputAlignmentNBest(
    Alignments &retAlign,
    const Syntax::KBestExtractor::Derivation &derivation,
    size_t startTarget)
{
  const Syntax::SHyperedge &shyperedge = derivation.edge->shyperedge;

  size_t totalTargetSize = 0;
  size_t startSource = shyperedge.head->pvertex->span.GetStartPos();

  const TargetPhrase &tp = *(shyperedge.translation);

  size_t thisSourceSize = CalcSourceSize(derivation);

  // position of each terminal word in translation rule, irrespective of alignment
  // if non-term, number is undefined
  vector<size_t> sourceOffsets(thisSourceSize, 0);
  vector<size_t> targetOffsets(tp.GetSize(), 0);

  const AlignmentInfo &aiNonTerm = shyperedge.translation->GetAlignNonTerm();
  vector<size_t> sourceInd2pos = aiNonTerm.GetSourceIndex2PosMap();
  const AlignmentInfo::NonTermIndexMap &targetPos2SourceInd = aiNonTerm.GetNonTermIndexMap();

  UTIL_THROW_IF2(sourceInd2pos.size() != derivation.subderivations.size(),
                 "Error");

  size_t targetInd = 0;
  for (size_t targetPos = 0; targetPos < tp.GetSize(); ++targetPos) {
    if (tp.GetWord(targetPos).IsNonTerminal()) {
      UTIL_THROW_IF2(targetPos >= targetPos2SourceInd.size(), "Error");
      size_t sourceInd = targetPos2SourceInd[targetPos];
      size_t sourcePos = sourceInd2pos[sourceInd];

      const Moses::Syntax::KBestExtractor::Derivation &subderivation =
        *derivation.subderivations[sourceInd];

      // calc source size
      size_t sourceSize =
          subderivation.edge->head->svertex.pvertex->span.GetNumWordsCovered();
      sourceOffsets[sourcePos] = sourceSize;

      // calc target size.
      // Recursively look thru child hypos
      size_t currStartTarget = startTarget + totalTargetSize;
      size_t targetSize = OutputAlignmentNBest(retAlign, subderivation,
                                               currStartTarget);
      targetOffsets[targetPos] = targetSize;

      totalTargetSize += targetSize;
      ++targetInd;
    } else {
      ++totalTargetSize;
    }
  }

  // convert position within translation rule to absolute position within
  // source sentence / output sentence
  ShiftOffsets(sourceOffsets, startSource);
  ShiftOffsets(targetOffsets, startTarget);

  // get alignments from this hypo
  const AlignmentInfo &aiTerm = shyperedge.translation->GetAlignTerm();

  // add to output arg, offsetting by source & target
  AlignmentInfo::const_iterator iter;
  for (iter = aiTerm.begin(); iter != aiTerm.end(); ++iter) {
    const std::pair<size_t,size_t> &align = *iter;
    size_t relSource = align.first;
    size_t relTarget = align.second;
    size_t absSource = sourceOffsets[relSource];
    size_t absTarget = targetOffsets[relTarget];

    pair<size_t, size_t> alignPoint(absSource, absTarget);
    pair<Alignments::iterator, bool> ret = retAlign.insert(alignPoint);
    UTIL_THROW_IF2(!ret.second, "Error");
  }

  return totalTargetSize;
}

void IOWrapper::OutputUnknowns(const std::set<Moses::Word> &unknowns,
                               long translationId)
{
  std::ostringstream out;
  for (std::set<Moses::Word>::const_iterator p = unknowns.begin();
       p != unknowns.end(); ++p) {
    out << *p;
  }
  out << std::endl;
  m_unknownsCollector->Write(translationId, out.str());
}

} // namespace

