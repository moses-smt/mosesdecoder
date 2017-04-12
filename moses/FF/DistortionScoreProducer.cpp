
#include "DistortionScoreProducer.h"
#include "FFState.h"
#include "moses/InputPath.h"
#include "moses/Range.h"
#include "moses/StaticData.h"
#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include "moses/FactorCollection.h"
#include <cmath>

using namespace std;

namespace Moses
{
struct DistortionState : public FFState {
  Range range;
  int first_gap;
  bool inSubordinateConjunction;
  DistortionState(const Range& wr, int fg, bool subord=false) : range(wr), first_gap(fg), inSubordinateConjunction(subord) {}

  size_t hash() const {
    return range.GetEndPos();
  }
  virtual bool operator==(const FFState& other) const {
    const DistortionState& o =
      static_cast<const DistortionState&>(other);
    return ( (range.GetEndPos() == o.range.GetEndPos()) && (inSubordinateConjunction == o.inSubordinateConjunction) );
  }

};

std::vector<const DistortionScoreProducer*> DistortionScoreProducer::s_staticColl;

DistortionScoreProducer::DistortionScoreProducer(const std::string &line)
  : StatefulFeatureFunction(1, line)
  , m_useSparse(false)
  , m_sparseDistance(false)
  , m_sparseSubordinate(false)
{
  s_staticColl.push_back(this);
  ReadParameters();
}

void DistortionScoreProducer::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "sparse") {
    m_useSparse = Scan<bool>(value);
  } else if (key == "sparse-distance") {
    m_sparseDistance = Scan<bool>(value);
  } else if (key == "sparse-input-factor") {
    m_sparseFactorTypeSource = Scan<FactorType>(value);
  } else if (key == "sparse-output-factor") {
    m_sparseFactorTypeTarget = Scan<FactorType>(value);
  } else if (key == "sparse-subordinate") {
    std::string subordinateConjunctionTag = Scan<std::string>(value);
    FactorCollection &factorCollection = FactorCollection::Instance();
    m_subordinateConjunctionTagFactor = factorCollection.AddFactor(subordinateConjunctionTag,false);
    m_sparseSubordinate = true;
  } else if (key == "sparse-subordinate-output-factor") {
    m_sparseFactorTypeTargetSubordinate = Scan<FactorType>(value);
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

const FFState* DistortionScoreProducer::EmptyHypothesisState(const InputType &input) const
{
  // fake previous translated phrase start and end
  size_t start = NOT_FOUND;
  size_t end = NOT_FOUND;
  if (input.m_frontSpanCoveredLength > 0) {
    // can happen with --continue-partial-translation
    start = 0;
    end = input.m_frontSpanCoveredLength -1;
  }
  return new DistortionState(
           Range(start, end),
           NOT_FOUND);
}

float
DistortionScoreProducer::
CalculateDistortionScore(const Hypothesis& hypo,
                         const Range &prev, const Range &curr, const int FirstGap)
{
  // if(!StaticData::Instance().UseEarlyDistortionCost()) {
  if(!hypo.GetManager().options()->reordering.use_early_distortion_cost) {
    return - (float) hypo.GetInput().ComputeDistortionDistance(prev, curr);
  } // else {

  /* Pay distortion score as soon as possible, from Moore and Quirk MT Summit 2007
     Definitions:
     S   : current source range
     S'  : last translated source phrase range
     S'' : longest fully-translated initial segment
  */

  int prefixEndPos = (int)FirstGap-1;
  if((int)FirstGap==-1)
    prefixEndPos = -1;

  // case1: S is adjacent to S'' => return 0
  if ((int) curr.GetStartPos() == prefixEndPos+1) {
    IFVERBOSE(4) std::cerr<< "MQ07disto:case1" << std::endl;
    return 0;
  }

  // case2: S is to the left of S' => return 2(length(S))
  if ((int) curr.GetEndPos() < (int) prev.GetEndPos()) {
    IFVERBOSE(4) std::cerr<< "MQ07disto:case2" << std::endl;
    return (float) -2*(int)curr.GetNumWordsCovered();
  }

  // case3: S' is a subsequence of S'' => return 2(nbWordBetween(S,S'')+length(S))
  if ((int) prev.GetEndPos() <= prefixEndPos) {
    IFVERBOSE(4) std::cerr<< "MQ07disto:case3" << std::endl;
    int z = (int)curr.GetStartPos()-prefixEndPos - 1;
    return (float) -2*(z + (int)curr.GetNumWordsCovered());
  }

  // case4: otherwise => return 2(nbWordBetween(S,S')+length(S))
  IFVERBOSE(4) std::cerr<< "MQ07disto:case4" << std::endl;
  return (float) -2*((int)curr.GetNumWordsBetween(prev) + (int)curr.GetNumWordsCovered());

}


FFState* DistortionScoreProducer::EvaluateWhenApplied(
  const Hypothesis& hypo,
  const FFState* prev_state,
  ScoreComponentCollection* out) const
{
  const DistortionState* prev = static_cast<const DistortionState*>(prev_state);
  bool subordinateConjunction = prev->inSubordinateConjunction;

  if (m_useSparse) {
    int jumpFromPos = prev->range.GetEndPos()+1;
    int jumpToPos = hypo.GetCurrSourceWordsRange().GetStartPos();
    size_t distance = std::abs( jumpFromPos - jumpToPos );

    const Sentence& sentence = static_cast<const Sentence&>(hypo.GetInput());

    StringPiece jumpFromSourceFactorPrev;
    StringPiece jumpFromSourceFactor;
    StringPiece jumpToSourceFactor;
    if (jumpFromPos < (int)sentence.GetSize()) {
      jumpFromSourceFactor = sentence.GetWord(jumpFromPos).GetFactor(m_sparseFactorTypeSource)->GetString();
    } else {
      jumpFromSourceFactor = "</s>";
    }
    if (jumpFromPos > 0) {
      jumpFromSourceFactorPrev = sentence.GetWord(jumpFromPos-1).GetFactor(m_sparseFactorTypeSource)->GetString();
    } else {
      jumpFromSourceFactorPrev = "<s>";
    }
    jumpToSourceFactor = sentence.GetWord(jumpToPos).GetFactor(m_sparseFactorTypeSource)->GetString();

    const TargetPhrase& currTargetPhrase = hypo.GetCurrTargetPhrase();
    StringPiece jumpToTargetFactor = currTargetPhrase.GetWord(0).GetFactor(m_sparseFactorTypeTarget)->GetString();

    util::StringStream featureName;

    // source factor (start position)
    featureName = util::StringStream();
    featureName << m_description << "_";
    if ( jumpToPos > jumpFromPos ) {
      featureName << "R";
    } else if ( jumpToPos < jumpFromPos ) {
      featureName << "L";
    } else {
      featureName << "M";
    }
    if (m_sparseDistance) {
      featureName << distance;
    }
    featureName << "_SFS_" << jumpFromSourceFactor;
    if (m_sparseSubordinate && subordinateConjunction) {
      featureName << "_SUBORD";
    }
    out->SparsePlusEquals(featureName.str(), 1);

    // source factor (start position minus 1)
    featureName = util::StringStream();
    featureName << m_description << "_";
    if ( jumpToPos > jumpFromPos ) {
      featureName << "R";
    } else if ( jumpToPos < jumpFromPos ) {
      featureName << "L";
    } else {
      featureName << "M";
    }
    if (m_sparseDistance) {
      featureName << distance;
    }
    featureName << "_SFP_" << jumpFromSourceFactorPrev;
    if (m_sparseSubordinate && subordinateConjunction) {
      featureName << "_SUBORD";
    }
    out->SparsePlusEquals(featureName.str(), 1);

    // source factor (end position)
    featureName = util::StringStream();
    featureName << m_description << "_";
    if ( jumpToPos > jumpFromPos ) {
      featureName << "R";
    } else if ( jumpToPos < jumpFromPos ) {
      featureName << "L";
    } else {
      featureName << "M";
    }
    if (m_sparseDistance) {
      featureName << distance;
    }
    featureName << "_SFE_" << jumpToSourceFactor;
    if (m_sparseSubordinate && subordinateConjunction) {
      featureName << "_SUBORD";
    }
    out->SparsePlusEquals(featureName.str(), 1);

    // target factor (end position)
    featureName = util::StringStream();
    featureName << m_description << "_";
    if ( jumpToPos > jumpFromPos ) {
      featureName << "R";
    } else if ( jumpToPos < jumpFromPos ) {
      featureName << "L";
    } else {
      featureName << "M";
    }
    if (m_sparseDistance) {
      featureName << distance;
    }
    featureName << "_TFE_" << jumpToTargetFactor;
    if (m_sparseSubordinate && subordinateConjunction) {
      featureName << "_SUBORD";
    }
    out->SparsePlusEquals(featureName.str(), 1);

    // relative source sentence position
    featureName = util::StringStream();
    featureName << m_description << "_";
    if ( jumpToPos > jumpFromPos ) {
      featureName << "R";
    } else if ( jumpToPos < jumpFromPos ) {
      featureName << "L";
    } else {
      featureName << "M";
    }
    if (m_sparseDistance) {
      featureName << distance;
    }
    size_t relativeSourceSentencePosBin = std::floor( 5 * (float)jumpFromPos / (sentence.GetSize()+1) );
    featureName << "_P_" << relativeSourceSentencePosBin;
    if (m_sparseSubordinate && subordinateConjunction) {
      featureName << "_SUBORD";
    }
    out->SparsePlusEquals(featureName.str(), 1);

    // source sentence length bin
    featureName = util::StringStream();
    featureName << m_description << "_";
    if ( jumpToPos > jumpFromPos ) {
      featureName << "R";
    } else if ( jumpToPos < jumpFromPos ) {
      featureName << "L";
    } else {
      featureName << "M";
    }
    if (m_sparseDistance) {
      featureName << distance;
    }
    size_t sourceSentenceLengthBin = 3;
    if (sentence.GetSize() < 15) {
      sourceSentenceLengthBin = 0;
    } else if (sentence.GetSize() < 23) {
      sourceSentenceLengthBin = 1;
    } else if (sentence.GetSize() < 33) {
      sourceSentenceLengthBin = 2;
    }
    featureName << "_SL_" << sourceSentenceLengthBin;
    if (m_sparseSubordinate && subordinateConjunction) {
      featureName << "_SUBORD";
    }
    out->SparsePlusEquals(featureName.str(), 1);

    if (m_sparseSubordinate) {
      for (size_t posT=0; posT<currTargetPhrase.GetSize(); ++posT) {
        const Word &wordT = currTargetPhrase.GetWord(posT);
        if (wordT[m_sparseFactorTypeTargetSubordinate] == m_subordinateConjunctionTagFactor) {
          subordinateConjunction = true;
        } else if (wordT[m_sparseFactorTypeTargetSubordinate]->GetString()[0] == 'V') {
          subordinateConjunction = false;
        }
      };
    }
  }

  const float distortionScore = CalculateDistortionScore(
                                  hypo,
                                  prev->range,
                                  hypo.GetCurrSourceWordsRange(),
                                  prev->first_gap);
  out->PlusEquals(this, distortionScore);

  DistortionState* state = new DistortionState(
    hypo.GetCurrSourceWordsRange(),
    hypo.GetWordsBitmap().GetFirstGapPos(),
    subordinateConjunction);

  return state;
}


}

