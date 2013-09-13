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
  //const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
  //  cur_hypo.GetCurrTargetPhrase().GetAlignNonTerm().GetNonTermIndexMap();
  
  //The Huck features. For a rule with source side:
  //   abXcdXef
  //We first have to split into blocks:
  // ab X cd X ef
  //Then we extract features based in the boundary words of the neighbouring blocks
  //For the block pair, we use the right word of the left block, and the left 
  //word of the right block.

  //Need to get blocks, and their alignment. Each block has a word range (on the 
  // on the source), a non-terminal flag, and  a set of alignment points in the target phrase

  //We need to be able to map source word position to target word position, as
  //much as possible (don't need interior of non-terminals). The alignment info
  //objects just give us the mappings between *rule* positions. So if we can 
  //map source word position to source rule position, and target rule position
  //to target word position, then we can map right through.

  size_t sourceStart = cur_hypo.GetCurrSourceRange().GetStartPos();
  size_t sourceSize = cur_hypo.GetCurrSourceRange().GetNumWordsCovered();

  vector<WordsRange> sourceNTSpans;
  for (size_t prevHypoId = 0; prevHypoId < cur_hypo.GetPrevHypos().size(); ++prevHypoId) {
    sourceNTSpans.push_back(cur_hypo.GetPrevHypo(prevHypoId)->GetCurrSourceRange());
  }
  //put in source order. Is this necessary?
  sort(sourceNTSpans.begin(), sourceNTSpans.end()); 
  //cerr << "Source NTs: ";
  //for (size_t i = 0; i < sourceNTSpans.size(); ++i) cerr << sourceNTSpans[i] << " ";
  //cerr << endl;

  typedef pair<WordsRange,bool> Block;//flag indicates NT
  vector<Block> sourceBlocks; 
  sourceBlocks.push_back(Block(cur_hypo.GetCurrSourceRange(),false));
  for (vector<WordsRange>::const_iterator i = sourceNTSpans.begin(); 
      i != sourceNTSpans.end(); ++i) {
    const WordsRange& prevHypoRange = *i;
    Block lastBlock = sourceBlocks.back();
    sourceBlocks.pop_back();
    //split this range into before NT, NT and after NT
    if (prevHypoRange.GetStartPos() > lastBlock.first.GetStartPos()) {
      sourceBlocks.push_back(Block(WordsRange(lastBlock.first.GetStartPos(),prevHypoRange.GetStartPos()-1),false));
    }
    sourceBlocks.push_back(Block(prevHypoRange,true));
    if (prevHypoRange.GetEndPos() < lastBlock.first.GetEndPos()) {
      sourceBlocks.push_back(Block(WordsRange(prevHypoRange.GetEndPos()+1,lastBlock.first.GetEndPos()), false));
    }
  }
  cerr << "Source Blocks: ";
  for (size_t i = 0; i < sourceBlocks.size(); ++i) cerr << sourceBlocks[i].first << " "
      << (sourceBlocks[i].second ? "NT" : "T") << " ";
  cerr << endl;

  //Mapping from source word to target rule position
  vector<size_t> sourceWordToTargetRulePos(sourceSize);
  map<size_t,size_t> alignMap;
  alignMap.insert(
    cur_hypo.GetCurrTargetPhrase().GetAlignTerm().begin(),
    cur_hypo.GetCurrTargetPhrase().GetAlignTerm().end());
  alignMap.insert(
    cur_hypo.GetCurrTargetPhrase().GetAlignNonTerm().begin(),
    cur_hypo.GetCurrTargetPhrase().GetAlignNonTerm().end());
  //vector<size_t> alignMapTerm = cur_hypo.GetCurrTargetPhrase().GetAlignNonTerm()
  size_t sourceRulePos = 0;
  //cerr << "SW->RP ";
  for (vector<Block>::const_iterator sourceBlockIt = sourceBlocks.begin(); 
    sourceBlockIt != sourceBlocks.end(); ++sourceBlockIt) {
    for (size_t sourceWordPos = sourceBlockIt->first.GetStartPos();
      sourceWordPos <= sourceBlockIt->first.GetEndPos(); ++sourceWordPos) {
      sourceWordToTargetRulePos[sourceWordPos - sourceStart] = alignMap[sourceRulePos];
   //   cerr << sourceWordPos - sourceStart << "-" << alignMap[sourceRulePos] << " ";
      if (! sourceBlockIt->second) {
        //T
        ++sourceRulePos;
      }
    }
    if ( sourceBlockIt->second) {
      //NT
      ++sourceRulePos;
    }
  }
  //cerr << endl;

  /**
  const InputPath* inputPath = cur_hypo.GetTranslationOption().GetInputPath();
  cerr << "IP phrase: " << inputPath->GetPhrase() << endl;
  cerr << "NTs ";
  for (NonTerminalSet::const_iterator i = inputPath->GetNonTerminalSet().begin();
    i != inputPath->GetNonTerminalSet().end(); ++i) {
    cerr << *i << " ";
  }
  cerr << endl;
  **/
  //Iterate through block pairs
  const Sentence& sentence = 
    dynamic_cast<const Sentence&>(cur_hypo.GetManager().GetSource());
  //const TargetPhrase& targetPhrase = cur_hypo.GetCurrTargetPhrase();
  for (size_t i = 0; i < sourceBlocks.size()-1; ++i) {
    Block& leftSourceBlock = sourceBlocks[i];
    Block& rightSourceBlock = sourceBlocks[i+1];
    size_t sourceLeftBoundaryPos = leftSourceBlock.first.GetEndPos();
    size_t sourceRightBoundaryPos = rightSourceBlock.first.GetStartPos();
    const Word& sourceLeftBoundaryWord = sentence.GetWord(sourceLeftBoundaryPos);
    const Word& sourceRightBoundaryWord = sentence.GetWord(sourceRightBoundaryPos);
    sourceLeftBoundaryPos -= sourceStart;
    sourceRightBoundaryPos -= sourceStart;
    
    // Need to figure out where these map to on the target.
    size_t targetLeftRulePos = 
      sourceWordToTargetRulePos[sourceLeftBoundaryPos];
    size_t targetRightRulePos = 
      sourceWordToTargetRulePos[sourceRightBoundaryPos];

    bool isMonotone = true;
    if ((sourceLeftBoundaryPos < sourceRightBoundaryPos  &&
          targetLeftRulePos > targetRightRulePos) ||
      ((sourceLeftBoundaryPos > sourceRightBoundaryPos  &&
          targetLeftRulePos < targetRightRulePos)))
    {
      isMonotone = false;
    }
    cerr << sourceLeftBoundaryWord.GetFactor(0)->GetString() <<
      "_" << sourceRightBoundaryWord.GetFactor(0)->GetString() << "_" 
      <<  (isMonotone ? "M" : "S") << endl;
  }
  cerr << endl;

  /*
  cerr << "NT align ";
  const AlignmentInfo& align = cur_hypo.GetCurrTargetPhrase().GetAlignNonTerm();
  for (AlignmentInfo::CollType::const_iterator i = align.begin(); i != align.end(); ++i) {
    cerr << i->first << "," << i->second << " ";
  }
  cerr << endl;

  cerr << "T align ";
  const AlignmentInfo& alignT = cur_hypo.GetCurrTargetPhrase().GetAlignTerm();
  for (AlignmentInfo::CollType::const_iterator i = alignT.begin(); i != alignT.end(); ++i) {
    cerr << i->first << "," << i->second << " ";
  }
  cerr << endl;
  */

  /*
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
  }*/

  return new SparseReorderingState();
}


}

