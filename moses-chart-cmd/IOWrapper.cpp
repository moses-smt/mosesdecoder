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
#include <boost/algorithm/string.hpp>
#include "IOWrapper.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/WordsRange.h"
#include "moses/StaticData.h"
#include "moses/InputFileStream.h"
#include "moses/Incremental.h"
#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/ChartTrellisPathList.h"
#include "moses/ChartTrellisPath.h"
#include "moses/ChartTrellisNode.h"
#include "moses/ChartTranslationOptions.h"
#include "moses/ChartHypothesis.h"
#include "moses/FeatureVector.h"
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/StatelessFeatureFunction.h"
#include "util/exception.hh"

using namespace std;
using namespace Moses;

namespace MosesChartCmd
{

IOWrapper::IOWrapper(const std::vector<FactorType>	&inputFactorOrder
                     , const std::vector<FactorType>	&outputFactorOrder
                     , const FactorMask							&inputFactorUsed
                     , size_t												nBestSize
                     , const std::string							&nBestFilePath
                     , const std::string							&inputFilePath)
  :m_inputFactorOrder(inputFactorOrder)
  ,m_outputFactorOrder(outputFactorOrder)
  ,m_inputFactorUsed(inputFactorUsed)
  ,m_outputSearchGraphStream(NULL)
  ,m_detailedTranslationReportingStream(NULL)
  ,m_detailedTreeFragmentsTranslationReportingStream(NULL)
  ,m_alignmentInfoStream(NULL)
  ,m_inputFilePath(inputFilePath)
  ,m_detailOutputCollector(NULL)
  ,m_detailTreeFragmentsOutputCollector(NULL)
  ,m_nBestOutputCollector(NULL)
  ,m_searchGraphOutputCollector(NULL)
  ,m_singleBestOutputCollector(NULL)
  ,m_alignmentInfoCollector(NULL)
{
  const StaticData &staticData = StaticData::Instance();

  if (m_inputFilePath.empty()) {
    m_inputStream = &std::cin;
  } else {
    m_inputStream = new InputFileStream(inputFilePath);
  }

  bool suppressSingleBestOutput = false;

  if (nBestSize > 0) {
    if (nBestFilePath == "-") {
      m_nBestOutputCollector = new Moses::OutputCollector(&std::cout);
      suppressSingleBestOutput = true;
    } else {
      m_nBestOutputCollector = new Moses::OutputCollector(new std::ofstream(nBestFilePath.c_str()));
      m_nBestOutputCollector->HoldOutputStream();
    }
  }

  if (!suppressSingleBestOutput) {
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

  if (staticData.IsDetailedTreeFragmentsTranslationReportingEnabled()) {
    const std::string &path = staticData.GetDetailedTreeFragmentsTranslationReportingFilePath();
    m_detailedTreeFragmentsTranslationReportingStream = new std::ofstream(path.c_str());
    m_detailTreeFragmentsOutputCollector = new Moses::OutputCollector(m_detailedTreeFragmentsTranslationReportingStream);
  }

  if (!staticData.GetAlignmentOutputFile().empty()) {
    m_alignmentInfoStream = new std::ofstream(staticData.GetAlignmentOutputFile().c_str());
    m_alignmentInfoCollector = new Moses::OutputCollector(m_alignmentInfoStream);
    UTIL_THROW_IF2(!m_alignmentInfoStream->good(),
    		"File for alignment output could not be opened: " << staticData.GetAlignmentOutputFile());
  }
}

IOWrapper::~IOWrapper()
{
  if (!m_inputFilePath.empty()) {
    delete m_inputStream;
  }
  delete m_outputSearchGraphStream;
  delete m_detailedTranslationReportingStream;
  delete m_detailedTreeFragmentsTranslationReportingStream;
  delete m_alignmentInfoStream;
  delete m_detailOutputCollector;
  delete m_nBestOutputCollector;
  delete m_searchGraphOutputCollector;
  delete m_singleBestOutputCollector;
  delete m_alignmentInfoCollector;
}

void IOWrapper::ResetTranslationId()
{
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

void IOWrapper::OutputBestHypo(const std::vector<const Factor*>&  mbrBestHypo, long /*translationId*/)
{
  for (size_t i = 0 ; i < mbrBestHypo.size() ; i++) {
    const Factor *factor = mbrBestHypo[i];
    UTIL_THROW_IF(factor == NULL, util::Exception,
    		"No factor at position " << i );

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

void IOWrapper::OutputTreeFragmentsTranslationOptions(std::ostream &out, ApplicationContext &applicationContext, const ChartHypothesis *hypo, const Sentence &sentence, long translationId)
{

  if (hypo != NULL) {
    OutputTranslationOption(out, applicationContext, hypo, sentence, translationId);

    const std::string key = "Tree";
    std::string value;
    bool hasProperty;
    const TargetPhrase &currTarPhr = hypo->GetCurrTargetPhrase();
    currTarPhr.GetProperty(key, value, hasProperty);

    out << " ||| ";
    if (hasProperty)
      out << " " << value;
    else
      out << " " << "noTreeInfo";
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
  UTIL_THROW_IF2(m_detailOutputCollector == NULL,
		  "No ouput file for detailed reports specified");
  m_detailOutputCollector->Write(translationId, out.str());
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
  m_detailTreeFragmentsOutputCollector->Write(translationId, out.str());
}

//DIMw
void IOWrapper::OutputDetailedAllTranslationReport(
  const ChartTrellisPathList &nBestList,
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
  UTIL_THROW_IF2(m_detailAllOutputCollector == NULL,
		  "No output file for details specified");
  m_detailAllOutputCollector->Write(translationId, out.str());
}

void IOWrapper::OutputBestHypo(const ChartHypothesis *hypo, long translationId)
{
  if (!m_singleBestOutputCollector)
    return;
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
  IOWrapper::FixPrecision(out);
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

void IOWrapper::OutputAllFeatureScores(const ScoreComponentCollection &features, std::ostream &out)
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
} // namespace

void IOWrapper::OutputFeatureScores( std::ostream& out, const ScoreComponentCollection &features, const FeatureFunction *ff, std::string &lastName )
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

void IOWrapper::OutputNBestList(const ChartTrellisPathList &nBestList, long translationId)
{
  std::ostringstream out;

  // Check if we're writing to std::cout.
  if (m_nBestOutputCollector->OutputIsCout()) {
    // Set precision only if we're writing the n-best list to cout.  This is to
    // preserve existing behaviour, but should probably be done either way.
    IOWrapper::FixPrecision(out);

    // Used to check StaticData's GetOutputHypoScore(), but it makes no sense with nbest output.
  }

  //bool includeAlignment = StaticData::Instance().NBestIncludesAlignment();
  bool includeWordAlignment = StaticData::Instance().PrintAlignmentInfoInNbest();

  ChartTrellisPathList::const_iterator iter;
  for (iter = nBestList.begin() ; iter != nBestList.end() ; ++iter) {
    const ChartTrellisPath &path = **iter;
    //cerr << path << endl << endl;

    Moses::Phrase outputPhrase = path.GetOutputPhrase();

    // delete 1st & last
    UTIL_THROW_IF2(outputPhrase.GetSize() < 2,
  		  "Output phrase should have contained at least 2 words (beginning and end-of-sentence)");

    outputPhrase.RemoveWord(0);
    outputPhrase.RemoveWord(outputPhrase.GetSize() - 1);

    // print the surface factor of the translation
    out << translationId << " ||| ";
    OutputSurface(out, outputPhrase, m_outputFactorOrder, false);
    out << " ||| ";

    // print the scores in a hardwired order
    // before each model type, the corresponding command-line-like name must be emitted
    // MERT script relies on this

    OutputAllFeatureScores(path.GetScoreBreakdown(), out);

    // total
    out << " ||| " << path.GetTotalScore();

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

    if (includeWordAlignment) {
      out << " ||| ";

      Alignments retAlign;

      const ChartTrellisNode &node = path.GetFinalNode();
      OutputAlignmentNBest(retAlign, node, 0);

      Alignments::const_iterator iter;
      for (iter = retAlign.begin(); iter != retAlign.end(); ++iter) {
        const pair<size_t, size_t> &alignPoint = *iter;
        out << alignPoint.first << "-" << alignPoint.second << " ";
      }
    }

    out << endl;
  }

  out <<std::flush;

  assert(m_nBestOutputCollector);
  m_nBestOutputCollector->Write(translationId, out.str());
}

void IOWrapper::OutputNBestList(const std::vector<search::Applied> &nbest, long translationId)
{
  std::ostringstream out;
  // wtf? copied from the original OutputNBestList
  if (m_nBestOutputCollector->OutputIsCout()) {
    IOWrapper::FixPrecision(out);
  }
  Phrase outputPhrase;
  ScoreComponentCollection features;
  for (std::vector<search::Applied>::const_iterator i = nbest.begin(); i != nbest.end(); ++i) {
    Incremental::PhraseAndFeatures(*i, outputPhrase, features);
    // <s> and </s>
    UTIL_THROW_IF2(outputPhrase.GetSize() < 2,
  		  "Output phrase should have contained at least 2 words (beginning and end-of-sentence)");

    outputPhrase.RemoveWord(0);
    outputPhrase.RemoveWord(outputPhrase.GetSize() - 1);
    out << translationId << " ||| ";
    OutputSurface(out, outputPhrase, m_outputFactorOrder, false);
    out << " ||| ";
    OutputAllFeatureScores(features, out);
    out << " ||| " << i->GetScore() << '\n';
  }
  out << std::flush;
  assert(m_nBestOutputCollector);
  m_nBestOutputCollector->Write(translationId, out.str());
}

void IOWrapper::FixPrecision(std::ostream &stream, size_t size)
{
  stream.setf(std::ios::fixed);
  stream.precision(size);
}

template <class T>
void ShiftOffsets(vector<T> &offsets, T shift)
{
  T currPos = shift;
  for (size_t i = 0; i < offsets.size(); ++i) {
    if (offsets[i] == 0) {
      offsets[i] = currPos;
      ++currPos;
    } else {
      currPos += offsets[i];
    }
  }
}

size_t CalcSourceSize(const Moses::ChartHypothesis *hypo)
{
  size_t ret = hypo->GetCurrSourceRange().GetNumWordsCovered();
  const std::vector<const ChartHypothesis*> &prevHypos = hypo->GetPrevHypos();
  for (size_t i = 0; i < prevHypos.size(); ++i) {
    size_t childSize = prevHypos[i]->GetCurrSourceRange().GetNumWordsCovered();
    ret -= (childSize - 1);
  }
  return ret;
}

size_t IOWrapper::OutputAlignmentNBest(Alignments &retAlign, const Moses::ChartTrellisNode &node, size_t startTarget)
{
  const ChartHypothesis *hypo = &node.GetHypothesis();

  size_t totalTargetSize = 0;
  size_t startSource = hypo->GetCurrSourceRange().GetStartPos();

  const TargetPhrase &tp = hypo->GetCurrTargetPhrase();

  size_t thisSourceSize = CalcSourceSize(hypo);

  // position of each terminal word in translation rule, irrespective of alignment
  // if non-term, number is undefined
  vector<size_t> sourceOffsets(thisSourceSize, 0);
  vector<size_t> targetOffsets(tp.GetSize(), 0);

  const ChartTrellisNode::NodeChildren &prevNodes = node.GetChildren();

  const AlignmentInfo &aiNonTerm = hypo->GetCurrTargetPhrase().GetAlignNonTerm();
  vector<size_t> sourceInd2pos = aiNonTerm.GetSourceIndex2PosMap();
  const AlignmentInfo::NonTermIndexMap &targetPos2SourceInd = aiNonTerm.GetNonTermIndexMap();

  UTIL_THROW_IF2(sourceInd2pos.size() != prevNodes.size(), "Error");

  size_t targetInd = 0;
  for (size_t targetPos = 0; targetPos < tp.GetSize(); ++targetPos) {
    if (tp.GetWord(targetPos).IsNonTerminal()) {
      UTIL_THROW_IF2(targetPos >= targetPos2SourceInd.size(), "Error");
      size_t sourceInd = targetPos2SourceInd[targetPos];
      size_t sourcePos = sourceInd2pos[sourceInd];

      const ChartTrellisNode &prevNode = *prevNodes[sourceInd];

      // calc source size
      size_t sourceSize = prevNode.GetHypothesis().GetCurrSourceRange().GetNumWordsCovered();
      sourceOffsets[sourcePos] = sourceSize;

      // calc target size.
      // Recursively look thru child hypos
      size_t currStartTarget = startTarget + totalTargetSize;
      size_t targetSize = OutputAlignmentNBest(retAlign, prevNode, currStartTarget);
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

void IOWrapper::OutputAlignment(vector< set<size_t> > &retAlignmentsS2T, const AlignmentInfo &ai)
{
  typedef std::vector< const std::pair<size_t,size_t>* > AlignVec;
  AlignVec alignments = ai.GetSortedAlignments();

  AlignVec::const_iterator it;
  for (it = alignments.begin(); it != alignments.end(); ++it) {
    const std::pair<size_t,size_t> &alignPoint = **it;

    UTIL_THROW_IF2(alignPoint.first >= retAlignmentsS2T.size(), "Error");
    pair<set<size_t>::iterator, bool> ret = retAlignmentsS2T[alignPoint.first].insert(alignPoint.second);
    UTIL_THROW_IF2(!ret.second, "Error");
  }
}

}

