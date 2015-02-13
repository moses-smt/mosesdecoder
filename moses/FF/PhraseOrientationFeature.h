//
// REFERENCE
// ---------
// When using this feature, please cite:
//
// Matthias Huck, Joern Wuebker, Felix Rietig, and Hermann Ney.
// A Phrase Orientation Model for Hierarchical Machine Translation.
// In ACL 2013 Eighth Workshop on Statistical Machine Translation (WMT 2013), pages 452-463, Sofia, Bulgaria, August 2013.
//

#pragma once

#include <bitset>
#include <string>
#include <vector>
#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include "moses/Factor.h"
#include "phrase-extract/extract-ghkm/PhraseOrientation.h"
#include "moses/PP/OrientationPhraseProperty.h"
#include <boost/unordered_set.hpp>


namespace Moses
{

class PhraseOrientationFeatureState : public FFState
{
public:

  friend class PhraseOrientationFeature;

  PhraseOrientationFeatureState(bool distinguishStates, bool useSparseWord, bool useSparseNT)
    : m_leftBoundaryNonTerminalL2RScores(3,0)
    , m_rightBoundaryNonTerminalR2LScores(3,0)
    , m_leftBoundaryNonTerminalL2RPossibleFutureOrientations(0x7)
    , m_rightBoundaryNonTerminalR2LPossibleFutureOrientations(0x7)
    , m_leftBoundaryRecursionGuard(false)
    , m_rightBoundaryRecursionGuard(false)
    , m_leftBoundaryIsSet(false)
    , m_rightBoundaryIsSet(false)
    , m_distinguishStates(distinguishStates)
    , m_useSparseWord(useSparseWord)
    , m_useSparseNT(useSparseNT)
  {}

  void SetLeftBoundaryL2R(const std::vector<float> &scores,
                          size_t heuristicScoreIndex,
                          std::bitset<3> &possibleFutureOrientations,
                          const Factor* leftBoundaryNonTerminalSymbol,
                          const PhraseOrientationFeatureState* prevState) {
    for (size_t i=0; i<3; ++i) {
      m_leftBoundaryNonTerminalL2RScores[i] = scores[i];
      m_leftBoundaryNonTerminalL2RPossibleFutureOrientations[i] = possibleFutureOrientations[i];
    }
    m_leftBoundaryNonTerminalL2RHeuristicScoreIndex = heuristicScoreIndex;
    m_leftBoundaryNonTerminalSymbol = leftBoundaryNonTerminalSymbol;
    m_leftBoundaryPrevState = prevState;
    m_leftBoundaryIsSet = true;
  }

  void SetRightBoundaryR2L(const std::vector<float> &scores,
                           size_t heuristicScoreIndex,
                           std::bitset<3> &possibleFutureOrientations,
                          const Factor* rightBoundaryNonTerminalSymbol,
                           const PhraseOrientationFeatureState* prevState) {
    for (size_t i=0; i<3; ++i) {
      m_rightBoundaryNonTerminalR2LScores[i] = scores[i];
      m_rightBoundaryNonTerminalR2LPossibleFutureOrientations[i] = possibleFutureOrientations[i];
    }
    m_rightBoundaryNonTerminalR2LHeuristicScoreIndex = heuristicScoreIndex;
    m_rightBoundaryNonTerminalSymbol = rightBoundaryNonTerminalSymbol;
    m_rightBoundaryPrevState = prevState;
    m_rightBoundaryIsSet = true;
  }

  float GetLeftBoundaryL2RScoreMono() const {
    return m_leftBoundaryNonTerminalL2RScores[0];
  }

  float GetLeftBoundaryL2RScoreSwap() const {
    return m_leftBoundaryNonTerminalL2RScores[1];
  }

  float GetLeftBoundaryL2RScoreDiscontinuous() const {
    return m_leftBoundaryNonTerminalL2RScores[2];
  }


  float GetRightBoundaryR2LScoreMono() const {
    return m_rightBoundaryNonTerminalR2LScores[0];
  }

  float GetRightBoundaryR2LScoreSwap() const {
    return m_rightBoundaryNonTerminalR2LScores[1];
  }

  float GetRightBoundaryR2LScoreDiscontinuous() const {
    return m_rightBoundaryNonTerminalR2LScores[2];
  }


  int Compare(const FFState& other) const {
    if (!m_distinguishStates) {
      return 0;
    }

    const PhraseOrientationFeatureState &otherState = static_cast<const PhraseOrientationFeatureState&>(other);

    if (!m_leftBoundaryIsSet && !otherState.m_leftBoundaryIsSet &&
        !m_rightBoundaryIsSet && !otherState.m_rightBoundaryIsSet) {
      return 0;
    }
    if (m_leftBoundaryIsSet && !otherState.m_leftBoundaryIsSet) {
      return 1;
    }
    if (!m_leftBoundaryIsSet && otherState.m_leftBoundaryIsSet) {
      return -1;
    }
    if (m_rightBoundaryIsSet && !otherState.m_rightBoundaryIsSet) {
      return 1;
    }
    if (!m_rightBoundaryIsSet && otherState.m_rightBoundaryIsSet) {
      return -1;
    }

    if (m_leftBoundaryIsSet) {
      int compareLeft = CompareLeftBoundaryRecursive(*this, otherState, m_useSparseNT);
      if (compareLeft != 0) {
        return compareLeft;
      }
    }
    if (m_rightBoundaryIsSet) {
      int compareRight = CompareRightBoundaryRecursive(*this, otherState, m_useSparseNT);
      if (compareRight != 0) {
        return compareRight;
      }
    }

    return 0;
  };

protected:

  static int CompareLeftBoundaryRecursive(const PhraseOrientationFeatureState& state, const PhraseOrientationFeatureState& otherState, bool useSparseNT) {
    if (!state.m_leftBoundaryIsSet && !otherState.m_leftBoundaryIsSet) {
      return 0;
    }
    if (state.m_leftBoundaryIsSet && !otherState.m_leftBoundaryIsSet) {
      return 1;
    }
    if (!state.m_leftBoundaryIsSet && otherState.m_leftBoundaryIsSet) {
      return -1;
    }

    if (useSparseNT) {
      if ( otherState.m_leftBoundaryNonTerminalSymbol < state.m_leftBoundaryNonTerminalSymbol ) {
        return 1;
      }
      if ( state.m_leftBoundaryNonTerminalSymbol < otherState.m_leftBoundaryNonTerminalSymbol ) {
        return -1;
      }
    }

    if ( otherState.m_leftBoundaryNonTerminalL2RHeuristicScoreIndex < state.m_leftBoundaryNonTerminalL2RHeuristicScoreIndex ) {
      return 1;
    }
    if ( state.m_leftBoundaryNonTerminalL2RHeuristicScoreIndex < otherState.m_leftBoundaryNonTerminalL2RHeuristicScoreIndex ) {
      return -1;
    }
    if ( Smaller(otherState.m_leftBoundaryNonTerminalL2RPossibleFutureOrientations, state.m_leftBoundaryNonTerminalL2RPossibleFutureOrientations) ) {
      return 1;
    }
    if ( Smaller(state.m_leftBoundaryNonTerminalL2RPossibleFutureOrientations, otherState.m_leftBoundaryNonTerminalL2RPossibleFutureOrientations) ) {
      return -1;
    }
    for (size_t i=0; i<state.m_leftBoundaryNonTerminalL2RScores.size(); ++i) {
      // compare only for possible future orientations
      // (possible future orientations of state and otherState are the same at this point due to the previous two conditional blocks)
      if (state.m_leftBoundaryNonTerminalL2RPossibleFutureOrientations[i]) { 
        if (state.m_leftBoundaryNonTerminalL2RScores[i] > otherState.m_leftBoundaryNonTerminalL2RScores[i]) {
          return 1;
        }
        if (state.m_leftBoundaryNonTerminalL2RScores[i] < otherState.m_leftBoundaryNonTerminalL2RScores[i]) {
          return -1;
        }
      }
    }

    if (state.m_leftBoundaryRecursionGuard && otherState.m_leftBoundaryRecursionGuard) {
      return 0;
    }
    if (state.m_leftBoundaryRecursionGuard && !otherState.m_leftBoundaryRecursionGuard) {
      return 1;
    }
    if (!state.m_leftBoundaryRecursionGuard && otherState.m_leftBoundaryRecursionGuard) {
      return -1;
    }

    const PhraseOrientationFeatureState *prevState = state.m_leftBoundaryPrevState;
    const PhraseOrientationFeatureState *otherPrevState = otherState.m_leftBoundaryPrevState;

    return CompareLeftBoundaryRecursive(*prevState, *otherPrevState, useSparseNT);
  };

  static int CompareRightBoundaryRecursive(const PhraseOrientationFeatureState& state, const PhraseOrientationFeatureState& otherState, bool useSparseNT) {
    if (!state.m_rightBoundaryIsSet && !otherState.m_rightBoundaryIsSet) {
      return 0;
    }
    if (state.m_rightBoundaryIsSet && !otherState.m_rightBoundaryIsSet) {
      return 1;
    }
    if (!state.m_rightBoundaryIsSet && otherState.m_rightBoundaryIsSet) {
      return -1;
    }

    if (useSparseNT) {
      if ( otherState.m_rightBoundaryNonTerminalSymbol < state.m_rightBoundaryNonTerminalSymbol ) {
        return 1;
      }
      if ( state.m_rightBoundaryNonTerminalSymbol < otherState.m_rightBoundaryNonTerminalSymbol ) {
        return -1;
      }
    }

    if ( otherState.m_rightBoundaryNonTerminalR2LHeuristicScoreIndex < state.m_rightBoundaryNonTerminalR2LHeuristicScoreIndex ) {
      return 1;
    }
    if ( state.m_rightBoundaryNonTerminalR2LHeuristicScoreIndex < otherState.m_rightBoundaryNonTerminalR2LHeuristicScoreIndex ) {
      return -1;
    }
    if ( Smaller(otherState.m_rightBoundaryNonTerminalR2LPossibleFutureOrientations, state.m_rightBoundaryNonTerminalR2LPossibleFutureOrientations) ) {
      return 1;
    }
    if ( Smaller(state.m_rightBoundaryNonTerminalR2LPossibleFutureOrientations, otherState.m_rightBoundaryNonTerminalR2LPossibleFutureOrientations) ) {
      return -1;
    }
    for (size_t i=0; i<state.m_rightBoundaryNonTerminalR2LScores.size(); ++i) {
      // compare only for possible future orientations
      // (possible future orientations of state and otherState are the same at this point due to the previous two conditional blocks)
      if ( state.m_rightBoundaryNonTerminalR2LPossibleFutureOrientations[i]) { 
        if (state.m_rightBoundaryNonTerminalR2LScores[i] > otherState.m_rightBoundaryNonTerminalR2LScores[i]) {
          return 1;
        }
        if (state.m_rightBoundaryNonTerminalR2LScores[i] < otherState.m_rightBoundaryNonTerminalR2LScores[i]) {
          return -1;
        }
      }
    }

    if (state.m_rightBoundaryRecursionGuard && otherState.m_rightBoundaryRecursionGuard) {
      return 0;
    }
    if (state.m_rightBoundaryRecursionGuard && !otherState.m_rightBoundaryRecursionGuard) {
      return 1;
    }
    if (!state.m_rightBoundaryRecursionGuard && otherState.m_rightBoundaryRecursionGuard) {
      return -1;
    }

    const PhraseOrientationFeatureState *prevState = state.m_rightBoundaryPrevState;
    const PhraseOrientationFeatureState *otherPrevState = otherState.m_rightBoundaryPrevState;

    return CompareRightBoundaryRecursive(*prevState, *otherPrevState, useSparseNT);
  };

  template<std::size_t N> static bool Smaller(const std::bitset<N>& x, const std::bitset<N>& y) {
    for (size_t i=0; i<N; ++i) {
      if (x[i] ^ y[i])
        return y[i];
    }
    return false;
  }

  std::vector<float> m_leftBoundaryNonTerminalL2RScores;
  std::vector<float> m_rightBoundaryNonTerminalR2LScores;

  size_t m_leftBoundaryNonTerminalL2RHeuristicScoreIndex;
  size_t m_rightBoundaryNonTerminalR2LHeuristicScoreIndex;

  std::bitset<3> m_leftBoundaryNonTerminalL2RPossibleFutureOrientations;
  std::bitset<3> m_rightBoundaryNonTerminalR2LPossibleFutureOrientations;

  bool m_leftBoundaryRecursionGuard;
  bool m_rightBoundaryRecursionGuard;
  bool m_leftBoundaryIsSet;
  bool m_rightBoundaryIsSet;
  const PhraseOrientationFeatureState* m_leftBoundaryPrevState;
  const PhraseOrientationFeatureState* m_rightBoundaryPrevState;
  const bool m_distinguishStates;
  const bool m_useSparseWord;
  const bool m_useSparseNT;
  const Factor* m_leftBoundaryNonTerminalSymbol;
  const Factor* m_rightBoundaryNonTerminalSymbol;
};



class PhraseOrientationFeature : public StatefulFeatureFunction
{
public:

  PhraseOrientationFeature(const std::string &line);

  ~PhraseOrientationFeature() {
  }

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new PhraseOrientationFeatureState(m_distinguishStates,m_useSparseWord,m_useSparseNT);
  }

  void SetParameter(const std::string& key, const std::string& value);
  
  void Load();

  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedFutureScore) const;

  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedFutureScore = NULL) const
  {};

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const
  {}

  FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const {
    UTIL_THROW2(GetScoreProducerDescription()
                << ": EvaluateWhenApplied(const Hypothesis&, ...) not implemented");
    return new PhraseOrientationFeatureState(m_distinguishStates,m_useSparseWord,m_useSparseNT);
  };

  FFState* EvaluateWhenApplied(
    const ChartHypothesis& cur_hypo,
    int featureID, // used to index the state in the previous hypotheses
    ScoreComponentCollection* accumulator) const;

protected:

  void LoadWordList(const std::string& filename,
                    boost::unordered_set<const Factor*>& list);

  void LookaheadScore(const OrientationPhraseProperty *orientationPhraseProperty, 
                      ScoreComponentCollection &scoreBreakdown, 
                      bool subtract=false) const;

  size_t GetHeuristicScoreIndex(const std::vector<float>& scores,
                                size_t weightsVectorOffset, 
                                const std::bitset<3> possibleFutureOrientations = 0x7) const;

  void LeftBoundaryL2RScoreRecursive(int featureID,
                                     const PhraseOrientationFeatureState *state,
                                     const std::bitset<3> orientation,
                                     std::vector<float>& newScores,
                                     ScoreComponentCollection* scoreBreakdown) const;

  void RightBoundaryR2LScoreRecursive(int featureID,
                                      const PhraseOrientationFeatureState *state,
                                      const std::bitset<3> orientation,
                                      std::vector<float>& newScores,
                                      ScoreComponentCollection* scoreBreakdown) const;

  void SparseWordL2RScore(const ChartHypothesis* hypo,
                          ScoreComponentCollection* scoreBreakdown,
                          const std::string* o) const;

  void SparseWordR2LScore(const ChartHypothesis* hypo,
                          ScoreComponentCollection* scoreBreakdown,
                          const std::string* o) const;

  void SparseNonTerminalL2RScore(const Factor* nonTerminalSymbol,
                                 ScoreComponentCollection* scoreBreakdown,
                                 const std::string* o) const;

  void SparseNonTerminalR2LScore(const Factor* nonTerminalSymbol,
                                 ScoreComponentCollection* scoreBreakdown,
                                 const std::string* o) const;

  const std::string* ToString(const Moses::GHKM::PhraseOrientation::REO_CLASS o) const;

  static const std::string MORIENT;
  static const std::string SORIENT;
  static const std::string DORIENT;

  std::string m_glueTargetLHSStr;
  const Factor* m_glueTargetLHS;
  bool m_distinguishStates;
  bool m_useSparseWord;
  bool m_useSparseNT;
  size_t m_offsetR2LScores;
  const std::vector<float> m_weightsVector;
  std::string m_filenameTargetWordList;
  boost::unordered_set<const Factor*> m_targetWordList;
  bool m_useTargetWordList;
  std::string m_filenameSourceWordList;
  boost::unordered_set<const Factor*> m_sourceWordList;
  bool m_useSourceWordList;

};


}

