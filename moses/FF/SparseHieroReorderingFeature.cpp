#include <iostream>

#include "moses/ChartHypothesis.h"
#include "moses/ChartManager.h"
#include "moses/FactorCollection.h"
#include "moses/Sentence.h"

#include "util/exception.hh"
#include "util/string_stream.hh"

#include "SparseHieroReorderingFeature.h"

using namespace std;

namespace Moses
{

SparseHieroReorderingFeature::SparseHieroReorderingFeature(const std::string &line)
  :StatelessFeatureFunction(0, line),
   m_type(SourceCombined),
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
  m_otherFactor = FactorCollection::Instance().AddFactor("##OTHER##");
  LoadVocabulary(m_sourceVocabFile, m_sourceVocab);
  LoadVocabulary(m_targetVocabFile, m_targetVocab);
}

void SparseHieroReorderingFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "input-factor") {
    m_sourceFactor = Scan<FactorType>(value);
  } else if (key == "output-factor") {
    m_targetFactor = Scan<FactorType>(value);
  } else if (key == "input-vocab-file") {
    m_sourceVocabFile = value;
  } else if (key == "output-vocab-file") {
    m_targetVocabFile = value;
  } else if (key == "type") {
    if (value == "SourceCombined") {
      m_type = SourceCombined;
    } else if (value == "SourceLeft") {
      m_type = SourceLeft;
    } else if (value == "SourceRight") {
      m_type = SourceRight;
    } else {
      UTIL_THROW(util::Exception, "Unknown sparse reordering type " << value);
    }
  } else {
    FeatureFunction::SetParameter(key, value);
  }
}

void SparseHieroReorderingFeature::LoadVocabulary(const std::string& filename, Vocab& vocab)
{
  if (filename.empty()) return;
  ifstream in(filename.c_str());
  UTIL_THROW_IF(!in, util::Exception, "Unable to open vocab file: " << filename);
  string line;
  while(getline(in,line)) {
    vocab.insert(FactorCollection::Instance().AddFactor(line));
  }
  in.close();
}

const Factor* SparseHieroReorderingFeature::GetFactor(const Word& word, const Vocab& vocab, FactorType factorType) const
{
  const Factor* factor = word.GetFactor(factorType);
  if (vocab.size() && vocab.find(factor) == vocab.end()) return m_otherFactor;
  return factor;
}

void SparseHieroReorderingFeature::EvaluateWhenApplied(
  const ChartHypothesis&  cur_hypo ,
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

  vector<Range> sourceNTSpans;
  for (size_t prevHypoId = 0; prevHypoId < cur_hypo.GetPrevHypos().size(); ++prevHypoId) {
    sourceNTSpans.push_back(cur_hypo.GetPrevHypo(prevHypoId)->GetCurrSourceRange());
  }
  //put in source order. Is this necessary?
  sort(sourceNTSpans.begin(), sourceNTSpans.end());
  //cerr << "Source NTs: ";
  //for (size_t i = 0; i < sourceNTSpans.size(); ++i) cerr << sourceNTSpans[i] << " ";
  //cerr << endl;

  typedef pair<Range,bool> Block;//flag indicates NT
  vector<Block> sourceBlocks;
  sourceBlocks.push_back(Block(cur_hypo.GetCurrSourceRange(),false));
  for (vector<Range>::const_iterator i = sourceNTSpans.begin();
       i != sourceNTSpans.end(); ++i) {
    const Range& prevHypoRange = *i;
    Block lastBlock = sourceBlocks.back();
    sourceBlocks.pop_back();
    //split this range into before NT, NT and after NT
    if (prevHypoRange.GetStartPos() > lastBlock.first.GetStartPos()) {
      sourceBlocks.push_back(Block(Range(lastBlock.first.GetStartPos(),prevHypoRange.GetStartPos()-1),false));
    }
    sourceBlocks.push_back(Block(prevHypoRange,true));
    if (prevHypoRange.GetEndPos() < lastBlock.first.GetEndPos()) {
      sourceBlocks.push_back(Block(Range(prevHypoRange.GetEndPos()+1,lastBlock.first.GetEndPos()), false));
    }
  }
  /*
  cerr << "Source Blocks: ";
  for (size_t i = 0; i < sourceBlocks.size(); ++i) cerr << sourceBlocks[i].first << " "
      << (sourceBlocks[i].second ? "NT" : "T") << " ";
  cerr << endl;
  */

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

  //Iterate through block pairs
  const Sentence& sentence =
    static_cast<const Sentence&>(cur_hypo.GetManager().GetSource());
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
          targetLeftRulePos < targetRightRulePos))) {
      isMonotone = false;
    }
    util::StringStream buf;
    buf << "h_"; //sparse reordering, Huck
    if (m_type == SourceLeft || m_type == SourceCombined) {
      buf << GetFactor(sourceLeftBoundaryWord,m_sourceVocab,m_sourceFactor)->GetString();
      buf << "_";
    }
    if (m_type == SourceRight || m_type == SourceCombined) {
      buf << GetFactor(sourceRightBoundaryWord,m_sourceVocab,m_sourceFactor)->GetString();
      buf << "_";
    }
    buf << (isMonotone ? "M" : "S");
    accumulator->PlusEquals(this,buf.str(), 1);
  }
//  cerr << endl;
}


}

