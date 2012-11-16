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
#include "IOWrapper.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/WordsRange.h"
#include "moses/StaticData.h"
#include "moses/DummyScoreProducers.h"
#include "moses/InputFileStream.h"
#include "moses/Incremental.h"
#include "moses/PhraseDictionary.h"
#include "moses/ChartTrellisPathList.h"
#include "moses/ChartTrellisPath.h"
#include "moses/ChartTrellisNode.h"
#include "moses/ChartTranslationOptions.h"
#include "moses/ChartHypothesis.h"
#include "moses/FeatureVector.h"

#include <boost/algorithm/string.hpp>


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
  ,m_alignmentInfoStream(NULL)
  ,m_inputFilePath(inputFilePath)
  ,m_detailOutputCollector(NULL)
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

  if (!staticData.GetAlignmentOutputFile().empty()) {
    m_alignmentInfoStream = new std::ofstream(staticData.GetAlignmentOutputFile().c_str());
    m_alignmentInfoCollector = new Moses::OutputCollector(m_alignmentInfoStream);
    CHECK(m_alignmentInfoStream->good());
  }
}

IOWrapper::~IOWrapper()
{
  if (!m_inputFilePath.empty()) {
    delete m_inputStream;
  }
  delete m_outputSearchGraphStream;
  delete m_detailedTranslationReportingStream;
  delete m_alignmentInfoStream;
  delete m_detailOutputCollector;
  delete m_nBestOutputCollector;
  delete m_searchGraphOutputCollector;
  delete m_singleBestOutputCollector;
  delete m_alignmentInfoCollector;
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

void IOWrapper::OutputBestHypo(const std::vector<const Factor*>&  mbrBestHypo, long /*translationId*/)
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

void OutputTranslationOptions(std::ostream &out, const ChartHypothesis *hypo, const Sentence &sentence, long translationId)
{
  static ApplicationContext applicationContext;

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

  const std::vector<const ChartHypothesis*> &prevHypos = hypo->GetPrevHypos();
  std::vector<const ChartHypothesis*>::const_iterator iter;
  for (iter = prevHypos.begin(); iter != prevHypos.end(); ++iter) {
    const ChartHypothesis *prevHypo = *iter;
    OutputTranslationOptions(out, prevHypo, sentence, translationId);
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
  OutputTranslationOptions(out, hypo, sentence, translationId);
  CHECK(m_detailOutputCollector);
  m_detailOutputCollector->Write(translationId, out.str());
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
    hypo->CreateOutputPhrase(outPhrase);
    
    // delete 1st & last
    CHECK(outPhrase.GetSize() >= 2);
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

void IOWrapper::OutputBestHypo(search::Applied applied, long translationId) {
  if (!m_singleBestOutputCollector) return;
  std::ostringstream out;
  IOWrapper::FixPrecision(out);
  if (StaticData::Instance().GetOutputHypoScore()) {
    out << applied.GetScore() << ' ';
  }
  Phrase outPhrase;
  Incremental::ToPhrase(applied, outPhrase);
  // delete 1st & last
  CHECK(outPhrase.GetSize() >= 2);
  outPhrase.RemoveWord(0);
  outPhrase.RemoveWord(outPhrase.GetSize() - 1);
  out << outPhrase.GetStringRep(StaticData::Instance().GetOutputFactorOrder());
  out << '\n';
  m_singleBestOutputCollector->Write(translationId, out.str());
}

void IOWrapper::OutputBestNone(long translationId) {
  if (!m_singleBestOutputCollector) return;
  if (StaticData::Instance().GetOutputHypoScore()) {
    m_singleBestOutputCollector->Write(translationId, "0 \n");
  } else {
    m_singleBestOutputCollector->Write(translationId, "\n");
  }
}

namespace {

void OutputSparseFeatureScores(std::ostream& out, const ScoreComponentCollection &features, const FeatureFunction *ff, std::string &lastName) {
  const StaticData &staticData = StaticData::Instance();
  bool labeledOutput = staticData.IsLabeledNBestList();
  const FVector scores = features.GetVectorForProducer( ff );

  // report weighted aggregate
  if (! ff->GetSparseFeatureReporting()) {
  	const FVector &weights = staticData.GetAllWeights().GetScoresVector();
  	if (labeledOutput && !boost::contains(ff->GetScoreProducerDescription(), ":"))
  		out << " " << ff->GetScoreProducerWeightShortName() << ":";
    out << " " << scores.inner_product(weights);
  }

  // report each feature
  else {
  	for(FVector::FNVmap::const_iterator i = scores.cbegin(); i != scores.cend(); i++) {
  		if (i->second != 0) { // do not report zero-valued features
  			if (labeledOutput)
  				out << " " << i->first << ":";
        out << " " << i->second;
      }
    }
  }
}

void WriteFeatures(const TranslationSystem &system, const ScoreComponentCollection &features, std::ostream &out) {
  bool labeledOutput = StaticData::Instance().IsLabeledNBestList();
  // lm
  const LMList& lml = system.GetLanguageModels();
  if (lml.size() > 0) {
    if (labeledOutput)
      out << "lm:";
    LMList::const_iterator lmi = lml.begin();
    for (; lmi != lml.end(); ++lmi) {
      out << " " << features.GetScoreForProducer(*lmi);
    }
  }

  std::string lastName = "";

  // output stateful sparse features
  const vector<const StatefulFeatureFunction*>& sff = system.GetStatefulFeatureFunctions();
  for( size_t i=0; i<sff.size(); i++ )
    if (sff[i]->GetNumScoreComponents() == ScoreProducer::unlimited)
      OutputSparseFeatureScores(out, features, sff[i], lastName);

  // translation components
  const vector<PhraseDictionaryFeature*>& pds = system.GetPhraseDictionaries();
  if (pds.size() > 0) {
    for( size_t i=0; i<pds.size(); i++ ) {
      size_t pd_numinputscore = pds[i]->GetNumInputScores();
      vector<float> scores = features.GetScoresForProducer( pds[i] );
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
  out << " " << features.GetScoreForProducer(system.GetWordPenaltyProducer());

  // generation
  const vector<GenerationDictionary*>& gds = system.GetGenerationDictionaries();
  if (gds.size() > 0) {
    for( size_t i=0; i<gds.size(); i++ ) {
      size_t pd_numinputscore = gds[i]->GetNumInputScores();
      vector<float> scores = features.GetScoresForProducer( gds[i] );
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

  // output stateless sparse features
  lastName = "";

  const vector<const StatelessFeatureFunction*>& slf = system.GetStatelessFeatureFunctions();
  for( size_t i=0; i<slf.size(); i++ ) {
    if (slf[i]->GetNumScoreComponents() == ScoreProducer::unlimited) {
      OutputSparseFeatureScores(out, features, slf[i], lastName);
    }
  }
} 

} // namespace

void IOWrapper::OutputNBestList(const ChartTrellisPathList &nBestList, const TranslationSystem* system, long translationId) {
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
    CHECK(outputPhrase.GetSize() >= 2);
    outputPhrase.RemoveWord(0);
    outputPhrase.RemoveWord(outputPhrase.GetSize() - 1);

    // print the surface factor of the translation
    out << translationId << " ||| ";
    OutputSurface(out, outputPhrase, m_outputFactorOrder, false);
    out << " ||| ";

    // print the scores in a hardwired order
    // before each model type, the corresponding command-line-like name must be emitted
    // MERT script relies on this

    WriteFeatures(*system, path.GetScoreBreakdown(), out);

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

void IOWrapper::OutputNBestList(const std::vector<search::Applied> &nbest, const TranslationSystem &system, long translationId) {
  std::ostringstream out;
  // wtf? copied from the original OutputNBestList
  if (m_nBestOutputCollector->OutputIsCout()) {
    IOWrapper::FixPrecision(out);
  }
  Phrase outputPhrase;
  ScoreComponentCollection features;
  for (std::vector<search::Applied>::const_iterator i = nbest.begin(); i != nbest.end(); ++i) {
    Incremental::PhraseAndFeatures(system, *i, outputPhrase, features);
    // <s> and </s>
    CHECK(outputPhrase.GetSize() >= 2);
    outputPhrase.RemoveWord(0);
    outputPhrase.RemoveWord(outputPhrase.GetSize() - 1);
    out << translationId << " ||| ";
    OutputSurface(out, outputPhrase, m_outputFactorOrder, false);
    out << " ||| ";
    WriteFeatures(system, features, out);
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
  for (size_t i = 0; i < offsets.size(); ++i) {
    shift += offsets[i];
    offsets[i] += shift;
  }
}

size_t IOWrapper::OutputAlignmentNBest(Alignments &retAlign, const Moses::ChartTrellisNode &node, size_t startTarget)
{
  const ChartHypothesis *hypo = &node.GetHypothesis();

  size_t totalTargetSize = 0;
  size_t startSource = hypo->GetCurrSourceRange().GetStartPos();

  const TargetPhrase &tp = hypo->GetCurrTargetPhrase();

  vector<size_t> sourceOffsets(hypo->GetCurrSourceRange().GetNumWordsCovered(), 0);
  vector<size_t> targetOffsets(tp.GetSize(), 0);

  const ChartTrellisNode::NodeChildren &prevNodes = node.GetChildren();

  const AlignmentInfo &aiNonTerm = hypo->GetCurrTargetPhrase().GetAlignNonTerm();
  vector<size_t> sourceInd2pos = aiNonTerm.GetSourceIndex2PosMap();
  const AlignmentInfo::NonTermIndexMap &targetPos2SourceInd = aiNonTerm.GetNonTermIndexMap();

  CHECK(sourceInd2pos.size() == prevNodes.size());

  size_t targetInd = 0;
  for (size_t targetPos = 0; targetPos < tp.GetSize(); ++targetPos) {
    if (tp.GetWord(targetPos).IsNonTerminal()) {
      CHECK(targetPos < targetPos2SourceInd.size());
      size_t sourceInd = targetPos2SourceInd[targetPos];
      size_t sourcePos = sourceInd2pos[sourceInd];

      const ChartTrellisNode &prevNode = *prevNodes[sourceInd];

      // 1st. calc source size
      size_t sourceSize = prevNode.GetHypothesis().GetCurrSourceRange().GetNumWordsCovered();
      sourceOffsets[sourcePos] = sourceSize;

      // 2nd. calc target size. Recursively look thru child hypos
      size_t currStartTarget = startTarget + totalTargetSize;
      size_t targetSize = OutputAlignmentNBest(retAlign, prevNode, currStartTarget);
      targetOffsets[targetPos] = targetSize;

      totalTargetSize += targetSize;
      ++targetInd;
    }
    else {
      ++totalTargetSize;
    }
  }

  // 3rd. shift offsets
  ShiftOffsets(sourceOffsets, startSource);
  ShiftOffsets(targetOffsets, startTarget);

  // get alignments from this hypo
  vector< set<size_t> > retAlignmentsS2T(hypo->GetCurrSourceRange().GetNumWordsCovered());
  const AlignmentInfo &aiTerm = hypo->GetCurrTargetPhrase().GetAlignTerm();
  OutputAlignment(retAlignmentsS2T, aiTerm);

  // add to output arg, offsetting by source & target
  for (size_t source = 0; source < retAlignmentsS2T.size(); ++source) {
    const set<size_t> &targets = retAlignmentsS2T[source];
    set<size_t>::const_iterator iter;
    for (iter = targets.begin(); iter != targets.end(); ++iter) {
      size_t target = *iter;
      pair<size_t, size_t> alignPoint(source + sourceOffsets[source]
                                     ,target + targetOffsets[target]);
      pair<Alignments::iterator, bool> ret = retAlign.insert(alignPoint);
      CHECK(ret.second);

    }
  }

  return totalTargetSize;
}

void IOWrapper::OutputAlignment(size_t translationId , const Moses::ChartHypothesis *hypo)
{
  ostringstream out;

  Alignments retAlign;
  OutputAlignment(retAlign, hypo, 0);

  // output alignments
  Alignments::const_iterator iter;
  for (iter = retAlign.begin(); iter != retAlign.end(); ++iter) {
    const pair<size_t, size_t> &alignPoint = *iter;
    out << alignPoint.first << "-" << alignPoint.second << " ";
  }
  out << endl;

  m_alignmentInfoCollector->Write(translationId, out.str());
}

size_t IOWrapper::OutputAlignment(Alignments &retAlign, const Moses::ChartHypothesis *hypo, size_t startTarget)
{
  size_t totalTargetSize = 0;
  size_t startSource = hypo->GetCurrSourceRange().GetStartPos();

  const TargetPhrase &tp = hypo->GetCurrTargetPhrase();

  vector<size_t> sourceOffsets(hypo->GetCurrSourceRange().GetNumWordsCovered(), 0);
  vector<size_t> targetOffsets(tp.GetSize(), 0);

  const vector<const ChartHypothesis*> &prevHypos = hypo->GetPrevHypos();

  const AlignmentInfo &aiNonTerm = hypo->GetCurrTargetPhrase().GetAlignNonTerm();
  vector<size_t> sourceInd2pos = aiNonTerm.GetSourceIndex2PosMap();
  const AlignmentInfo::NonTermIndexMap &targetPos2SourceInd = aiNonTerm.GetNonTermIndexMap();

  CHECK(sourceInd2pos.size() == prevHypos.size());

  size_t targetInd = 0;
  for (size_t targetPos = 0; targetPos < tp.GetSize(); ++targetPos) {
    if (tp.GetWord(targetPos).IsNonTerminal()) {
      CHECK(targetPos < targetPos2SourceInd.size());
      size_t sourceInd = targetPos2SourceInd[targetPos];
      size_t sourcePos = sourceInd2pos[sourceInd];

      const ChartHypothesis *prevHypo = prevHypos[sourceInd];

      // 1st. calc source size
      size_t sourceSize = prevHypo->GetCurrSourceRange().GetNumWordsCovered();
      sourceOffsets[sourcePos] = sourceSize;

      // 2nd. calc target size. Recursively look thru child hypos
      size_t currStartTarget = startTarget + totalTargetSize;
      size_t targetSize = OutputAlignment(retAlign, prevHypo, currStartTarget);
      targetOffsets[targetPos] = targetSize;

      totalTargetSize += targetSize;
      ++targetInd;
    }
    else {
      ++totalTargetSize;
    }
  }

  // 3rd. shift offsets
  ShiftOffsets(sourceOffsets, startSource);
  ShiftOffsets(targetOffsets, startTarget);

  // get alignments from this hypo
  vector< set<size_t> > retAlignmentsS2T(hypo->GetCurrSourceRange().GetNumWordsCovered());
  const AlignmentInfo &aiTerm = hypo->GetCurrTargetPhrase().GetAlignTerm();
  OutputAlignment(retAlignmentsS2T, aiTerm);

  // add to output arg, offsetting by source & target
  for (size_t source = 0; source < retAlignmentsS2T.size(); ++source) {
    const set<size_t> &targets = retAlignmentsS2T[source];
    set<size_t>::const_iterator iter;
    for (iter = targets.begin(); iter != targets.end(); ++iter) {
      size_t target = *iter;
      pair<size_t, size_t> alignPoint(source + sourceOffsets[source]
                                     ,target + targetOffsets[target]);
      pair<Alignments::iterator, bool> ret = retAlign.insert(alignPoint);
      CHECK(ret.second);

    }
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

    CHECK(alignPoint.first < retAlignmentsS2T.size());
    pair<set<size_t>::iterator, bool> ret = retAlignmentsS2T[alignPoint.first].insert(alignPoint.second);
    CHECK(ret.second);
  }
}

}

