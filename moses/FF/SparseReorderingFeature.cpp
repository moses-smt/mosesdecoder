#include <iostream>

#include "moses/ChartHypothesis.h"
#include "moses/ChartManager.h"
#include "moses/Sentence.h"

#include "util/exception.hh"

#include "SparseReorderingFeature.h"

using namespace std;

namespace Moses
{

SparseReorderingFeature::SparseReorderingFeature(const std::string &line)
  :StatefulFeatureFunction("StatefulFeatureFunction",0, line),
  m_sourceFactor(0),
  m_targetFactor(0),
  m_sourceVocabFile(""),
  m_targetVocabFile("")
{

  /*
    Configuration of features.
      factor - Which factor should it apply to
      type - what type of sparse reordering feature. e.g. block (modelled on Matthias
        Huck's EAMT 2012 features)
      word - which words to include, e.g. src_bdry, src_all, tgt_bdry , ...
      vocab - vocab file to limit it to
      orientation - e.g. lr, etc.
  */
  cerr << "Constructing a Sparse Reordering feature" << endl;
  ReadParameters();
  LoadVocabulary(m_sourceVocabFile, m_sourceVocab);
  LoadVocabulary(m_targetVocabFile, m_targetVocab);
}

void SparseReorderingFeature::SetParameter(const std::string& key, const std::string& value) {
  if (key == "input-factor") {
    m_sourceFactor = Scan<FactorType>(value);
  } else if (key == "output-factor") {
    m_targetFactor = Scan<FactorType>(value);
  } else if (key == "input-vocab-file") {
    m_sourceVocabFile = value;
  } else if (key == "output-vocab-file") {
    m_targetVocabFile = value;
  } else {
    FeatureFunction::SetParameter(key, value);
  }
}

void SparseReorderingFeature::LoadVocabulary(const std::string& filename, boost::unordered_set<std::string>& vocab)
{
  if (filename.empty()) return;
  ifstream in(filename.c_str());
  UTIL_THROW_IF(!in, util::Exception, "Unable to open vocab file: " << filename);
}

static void AddFeatureWordPair(const string& prefix, const string& suffix,
  const Word& word1, const Word& word2, ScoreComponentCollection* accumulator, FactorType factor = 0) {
  stringstream buf;
  buf << prefix << word1[factor]->GetString() << "_" << word2[factor]->GetString() << suffix;
  accumulator->SparsePlusEquals(buf.str(), 1);
}
  

void SparseReorderingFeature::AddNonTerminalPairFeatures(
  const Sentence& sentence, const WordsRange& nt1, const WordsRange& nt2,
    bool isMonotone, ScoreComponentCollection* accumulator) const {
  //TODO: remove string concatenation
  const static string monotone = "_M";
  const static string swap = "_S";
  const static string prefixes[] = 
    { "srf_slslw_", "srf_slsrw_", "srf_srslw_", "srf_srsrw_"};

  string direction = isMonotone ? monotone : swap;
  AddFeatureWordPair(prefixes[0], direction,
     sentence.GetWord(nt1.GetStartPos()), sentence.GetWord(nt2.GetStartPos()), accumulator);
  AddFeatureWordPair(prefixes[1], direction,
     sentence.GetWord(nt1.GetStartPos()), sentence.GetWord(nt2.GetEndPos()),  accumulator);
  AddFeatureWordPair(prefixes[2], direction,
     sentence.GetWord(nt1.GetEndPos()), sentence.GetWord(nt2.GetStartPos()), accumulator);
  AddFeatureWordPair(prefixes[3], direction,
     sentence.GetWord(nt1.GetEndPos()), sentence.GetWord(nt2.GetStartPos()), accumulator);
}

FFState* SparseReorderingFeature::EvaluateChart(
  const ChartHypothesis&  cur_hypo ,
  int  featureID /*- used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  // get index map for underlying hypotheses
  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
    cur_hypo.GetCurrTargetPhrase().GetAlignNonTerm().GetNonTermIndexMap();
  
  //The Huck features. For a rule with source side:
  //   abXcdXef
  //We first have to split into blocks:
  // ab X cd X ef
  //Then we extract features based in the boundary words of the neighbouring blocks
  //For the block pair, we use the right word of the left block, and the left 
  //word of the right block.

  //Need to get blocks, and their alignment. Each block has a word range (on the 
  // on the source), a non-terminal flag, and  a set of alignment points in the target phrase

  vector<WordsRange> sourceNTSpans;
  for (size_t prevHypoId = 0; prevHypoId < cur_hypo.GetPrevHypos().size(); ++prevHypoId) {
    sourceNTSpans.push_back(cur_hypo.GetPrevHypo(prevHypoId)->GetCurrSourceRange());
  }
  sort(sourceNTSpans.begin(), sourceNTSpans.end()); //put in source order
  cerr << "Source NTs: ";
  for (size_t i = 0; i < sourceNTSpans.size(); ++i) cerr << sourceNTSpans[i] << " ";
  cerr << endl;

  vector<WordsRange> blocks;
  blocks.push_back(cur_hypo.GetCurrSourceRange());
  for (vector<WordsRange>::const_iterator i = sourceNTSpans.begin(); 
      i != sourceNTSpans.end(); ++i) {
    const WordsRange& prevHypoRange = *i;
    WordsRange lastRange = blocks.back();
    blocks.pop_back();
    //split this range into before NT, NT and after NT
    if (prevHypoRange.GetStartPos() > lastRange.GetStartPos()) {
      blocks.push_back(WordsRange(lastRange.GetStartPos(),prevHypoRange.GetStartPos()-1));
    }
    blocks.push_back(prevHypoRange);
    if (prevHypoRange.GetEndPos() < lastRange.GetEndPos()) {
      blocks.push_back(WordsRange(prevHypoRange.GetEndPos()+1,lastRange.GetEndPos()));
    }
  }
  cerr << "Blocks: ";
  for (size_t i = 0; i < blocks.size(); ++i) cerr << blocks[i] << " ";
  cerr << endl;

  //this currently doesn't work
  const InputPath* inputPath = cur_hypo.GetTranslationOption().GetInputPath();
  //The phrase is always dangling
  //cerr << "IP: phrase " << inputPath << endl;
  /*
  cerr << "NTs ";
  for (NonTerminalSet::const_iterator i = inputPath->GetNonTerminalSet().begin();
    i != inputPath->GetNonTerminalSet().end(); ++i) {
    cerr << *i << " ";
  }
  cerr << endl;
  */

  //Get mapping from target to source, in target order
  vector<pair<size_t, size_t> > targetNTs; //(srcIdx,targetPos)
  for (size_t targetIdx = 0; targetIdx < nonTermIndexMap.size(); ++targetIdx) {
    size_t srcNTIdx;
    if ((srcNTIdx = nonTermIndexMap[targetIdx]) == NOT_FOUND) continue;
    targetNTs.push_back(pair<size_t,size_t> (srcNTIdx,targetIdx));
  }
  //Add features for pairs of non-terminals
  for (size_t i = 0; i < targetNTs.size(); ++i) {
    for (size_t j = i+1; j < targetNTs.size(); ++j) {
      size_t src1 = targetNTs[i].first;
      size_t src2 = targetNTs[j].first;
      //NT pair (src1,src2) maps to (i,j)
      bool isMonotone = true;
      if ((src1 < src2 && i > j) || (src1 > src2 && i < j)) isMonotone = false;
      //NB: should throw bad_cast for Lattice input
      const Sentence& sentence = 
        dynamic_cast<const Sentence&>(cur_hypo.GetManager().GetSource());
      AddNonTerminalPairFeatures(sentence,
        cur_hypo.GetPrevHypo(src1)->GetCurrSourceRange(),
        cur_hypo.GetPrevHypo(src2)->GetCurrSourceRange(),
        isMonotone,
        accumulator);
    }
  }

  return new SparseReorderingState();
}


}

