//
// REFERENCE
// ---------
// When using this feature, please cite:
//
// Matthias Huck, Joern Wuebker, Felix Rietig, and Hermann Ney.
// A Phrase Orientation Model for Hierarchical Machine Translation.
// In ACL 2013 Eighth Workshop on Statistical Machine Translation (WMT 2013), pages 452-463, Sofia, Bulgaria, August 2013.
//

#include "PhraseOrientationFeature.h"
#include "moses/InputFileStream.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/StaticData.h"
#include "moses/Hypothesis.h"
#include "moses/ChartHypothesis.h"
#include "moses/ChartManager.h"
#include "moses/FactorCollection.h"
#include "moses/PP/OrientationPhraseProperty.h"
#include "phrase-extract/extract-ghkm/Alignment.h"


namespace Moses
{

PhraseOrientationFeature::PhraseOrientationFeature(const std::string &line)
  : StatefulFeatureFunction(6, line)
  , m_glueTargetLHSStr("Q")
  , m_glueTargetLHS(true)
  , m_offsetR2LScores(0)
{
  VERBOSE(1, "Initializing feature " << GetScoreProducerDescription() << " ...");
  ReadParameters();
  FactorCollection &fc = FactorCollection::Instance();
  const Factor *factor = fc.AddFactor(m_glueTargetLHSStr, true);
  m_glueTargetLHS.SetFactor(0, factor);
  m_offsetR2LScores = m_numScoreComponents / 2;
  VERBOSE(1, " Done." << std::endl);
}

void PhraseOrientationFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "glueTargetLHS") {
    m_glueTargetLHSStr = value;
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}


FFState* PhraseOrientationFeature::EvaluateWhenApplied(
  const ChartHypothesis& hypo,
  int featureID, // used to index the state in the previous hypotheses
  ScoreComponentCollection* accumulator) const
{
  // Dense scores
  std::vector<float> newScores(m_numScoreComponents,0);

  // State: ignored wrt. recombination; used to propagate orientation probabilities in case of boundary non-terminals
  PhraseOrientationFeatureState *state = new PhraseOrientationFeatureState();

  // Read Orientation property
  const TargetPhrase &currTarPhr = hypo.GetCurrTargetPhrase();
  const Word &currTarPhrLHS = currTarPhr.GetTargetLHS();
  const Phrase *currSrcPhr = currTarPhr.GetRuleSource();
//  const Factor* targetLHS = currTarPhr.GetTargetLHS()[0];
//  bool isGlueGrammarRule = false;

  IFFEATUREVERBOSE(2) {
    FEATUREVERBOSE(2, *currSrcPhr << std::endl);
    FEATUREVERBOSE(2, currTarPhr << std::endl);

    for (AlignmentInfo::const_iterator it=currTarPhr.GetAlignTerm().begin();
         it!=currTarPhr.GetAlignTerm().end(); ++it) {
      FEATUREVERBOSE(2, "alignTerm " << it->first << " " << it->second << std::endl);
    }

    for (AlignmentInfo::const_iterator it=currTarPhr.GetAlignNonTerm().begin();
         it!=currTarPhr.GetAlignNonTerm().end(); ++it) {
      FEATUREVERBOSE(2, "alignNonTerm " << it->first << " " << it->second << std::endl);
    }
  }

  // Initialize phrase orientation scoring object
  Moses::GHKM::PhraseOrientation phraseOrientation(currSrcPhr->GetSize(), currTarPhr.GetSize(),
      currTarPhr.GetAlignTerm(), currTarPhr.GetAlignNonTerm());

  // Get index map for underlying hypotheses
  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
    currTarPhr.GetAlignNonTerm().GetNonTermIndexMap();

  // Determine & score orientations

  for (AlignmentInfo::const_iterator it=currTarPhr.GetAlignNonTerm().begin();
       it!=currTarPhr.GetAlignNonTerm().end(); ++it) {
    size_t sourceIndex = it->first;
    size_t targetIndex = it->second;
    size_t nonTermIndex = nonTermIndexMap[targetIndex];

    FEATUREVERBOSE(2, "Scoring nonTermIndex= " << nonTermIndex << " targetIndex= " << targetIndex << " sourceIndex= " << sourceIndex << std::endl);

    // consult subderivation
    const ChartHypothesis *prevHypo = hypo.GetPrevHypo(nonTermIndex);
    const TargetPhrase &prevTarPhr = prevHypo->GetCurrTargetPhrase();

    if (const PhraseProperty *property = prevTarPhr.GetProperty("Orientation")) {
      const OrientationPhraseProperty *orientationPhraseProperty = static_cast<const OrientationPhraseProperty*>(property);

      FEATUREVERBOSE(5, "orientationPhraseProperty: "
                     << "L2R_Mono "    << orientationPhraseProperty->GetLeftToRightProbabilityMono()
                     << " L2R_Swap "   << orientationPhraseProperty->GetLeftToRightProbabilitySwap()
                     << " L2R_Dright " << orientationPhraseProperty->GetLeftToRightProbabilityDright()
                     << " L2R_Dleft "  << orientationPhraseProperty->GetLeftToRightProbabilityDleft()
                     << " R2L_Mono "   << orientationPhraseProperty->GetRightToLeftProbabilityMono()
                     << " R2L_Swap "   << orientationPhraseProperty->GetRightToLeftProbabilitySwap()
                     << " R2L_Dright " << orientationPhraseProperty->GetRightToLeftProbabilityDright()
                     << " R2L_Dleft "  << orientationPhraseProperty->GetRightToLeftProbabilityDleft()
                     << std::endl);

      const PhraseOrientationFeatureState* prevState =
        static_cast<const PhraseOrientationFeatureState*>(prevHypo->GetFFState(featureID));


      // LEFT-TO-RIGHT DIRECTION

      Moses::GHKM::PhraseOrientation::REO_CLASS l2rOrientation = phraseOrientation.GetOrientationInfo(sourceIndex,sourceIndex,Moses::GHKM::PhraseOrientation::REO_DIR_L2R);

      IFFEATUREVERBOSE(2) {
        FEATUREVERBOSE(2, "l2rOrientation ");
        switch (l2rOrientation) {
        case Moses::GHKM::PhraseOrientation::REO_CLASS_LEFT:
          FEATUREVERBOSE2(2, "mono" << std::endl);
          break;
        case Moses::GHKM::PhraseOrientation::REO_CLASS_RIGHT:
          FEATUREVERBOSE2(2, "swap" << std::endl);
          break;
        case Moses::GHKM::PhraseOrientation::REO_CLASS_DLEFT:
          FEATUREVERBOSE2(2, "dleft" << std::endl);
          break;
        case Moses::GHKM::PhraseOrientation::REO_CLASS_DRIGHT:
          FEATUREVERBOSE2(2, "dright" << std::endl);
          break;
        case Moses::GHKM::PhraseOrientation::REO_CLASS_UNKNOWN:
          // modelType == Moses::GHKM::PhraseOrientation::REO_MSLR
          FEATUREVERBOSE2(2, "unknown->dleft" << std::endl);
          break;
        default:
          UTIL_THROW2(GetScoreProducerDescription()
                      << ": Unsupported orientation type.");
          break;
        }
      }

      bool delayedScoringL2R = false;

      if ( ((targetIndex == 0) || !phraseOrientation.TargetSpanIsAligned(0,targetIndex)) // boundary non-terminal in rule-initial position (left boundary)
           && (currTarPhrLHS != m_glueTargetLHS) ) { // and not glue rule
        // delay left-to-right scoring

        FEATUREVERBOSE(3, "Left boundary");
        if (targetIndex != 0) {
          FEATUREVERBOSE2(3, " (with targetIndex!=0)");
        }
        FEATUREVERBOSE2(3, std::endl);

        bool previousSourceSpanIsAligned  = ( (sourceIndex > 0) && phraseOrientation.SourceSpanIsAligned(0,sourceIndex-1) );
        bool followingSourceSpanIsAligned = ( (sourceIndex < currSrcPhr->GetSize()-1) && phraseOrientation.SourceSpanIsAligned(sourceIndex,currSrcPhr->GetSize()-1) );

        FEATUREVERBOSE(4, "previousSourceSpanIsAligned = " << previousSourceSpanIsAligned << std::endl);
        FEATUREVERBOSE(4, "followingSourceSpanIsAligned = " << followingSourceSpanIsAligned << std::endl;);

        if (previousSourceSpanIsAligned && followingSourceSpanIsAligned) {
          // discontinuous
          l2rOrientation = Moses::GHKM::PhraseOrientation::REO_CLASS_DLEFT;
        } else {
          FEATUREVERBOSE(3, "Delaying left-to-right scoring" << std::endl);

          delayedScoringL2R = true;
          std::bitset<3> possibleFutureOrientationsL2R(0x7);
          possibleFutureOrientationsL2R[0] = !previousSourceSpanIsAligned;
          possibleFutureOrientationsL2R[1] = !followingSourceSpanIsAligned;

          // add heuristic scores

          std::vector<float> weightsVector = StaticData::Instance().GetAllWeights().GetScoresForProducer(this);
          std::vector<float> scoresL2R;
          scoresL2R.push_back( std::log(orientationPhraseProperty->GetLeftToRightProbabilityMono()) );
          scoresL2R.push_back( std::log(orientationPhraseProperty->GetLeftToRightProbabilitySwap()) );
          scoresL2R.push_back( std::log(orientationPhraseProperty->GetLeftToRightProbabilityDiscontinuous()) );
          std::vector<float> weightedScoresL2R;
          for ( size_t i=0; i<3; ++i ) {
            weightedScoresL2R.push_back( weightsVector[i] * scoresL2R[i] );
          }

          size_t heuristicScoreIndex = 0;
          for (size_t i=1; i<3; ++i) {
            if (possibleFutureOrientationsL2R[i]) {
              if (weightedScoresL2R[i] > weightedScoresL2R[heuristicScoreIndex]) {
                heuristicScoreIndex = i;
              }
            }
          }

          IFFEATUREVERBOSE(5) {
            FEATUREVERBOSE(5, "Heuristic score computation (L2R): "
                           << "heuristicScoreIndex= " << heuristicScoreIndex);
            for (size_t i=0; i<3; ++i)
              FEATUREVERBOSE2(5, " weightsVector[" << i << "]= " << weightsVector[i]);
            for (size_t i=0; i<3; ++i)
              FEATUREVERBOSE2(5, " scoresL2R[" << i << "]= " << scoresL2R[i]);
            for (size_t i=0; i<3; ++i)
              FEATUREVERBOSE2(5, " weightedScoresL2R[" << i << "]= " << weightedScoresL2R[i]);
            for (size_t i=0; i<3; ++i)
              FEATUREVERBOSE2(5, " possibleFutureOrientationsL2R[" << i << "]= " << possibleFutureOrientationsL2R[i]);
            if ( possibleFutureOrientationsL2R == 0x7 ) {
              FEATUREVERBOSE2(5, " (all orientations possible)");
            }
            FEATUREVERBOSE2(5, std::endl);
          }

          newScores[heuristicScoreIndex] += scoresL2R[heuristicScoreIndex];
          state->SetLeftBoundaryL2R(scoresL2R, heuristicScoreIndex, possibleFutureOrientationsL2R, prevState);

          if ( (possibleFutureOrientationsL2R & prevState->m_leftBoundaryNonTerminalL2RPossibleFutureOrientations) == 0x4 ) {
            // recursive: discontinuous orientation
            FEATUREVERBOSE(5, "previous state: L2R discontinuous orientation "
                           << possibleFutureOrientationsL2R << " & " << prevState->m_leftBoundaryNonTerminalL2RPossibleFutureOrientations
                           << " = " << (possibleFutureOrientationsL2R & prevState->m_leftBoundaryNonTerminalL2RPossibleFutureOrientations)
                           << std::endl);
            LeftBoundaryL2RScoreRecursive(featureID, prevState, 0x4, newScores);
            state->m_leftBoundaryRecursionGuard = true; // prevent subderivation from being scored recursively multiple times
          }
        }
      }

      if (!delayedScoringL2R) {
        switch (l2rOrientation) {
        case Moses::GHKM::PhraseOrientation::REO_CLASS_LEFT:
          newScores[0] += std::log(orientationPhraseProperty->GetLeftToRightProbabilityMono());
          // if sub-derivation has left-boundary non-terminal:
          // add recursive actual score of boundary non-terminal from subderivation
          LeftBoundaryL2RScoreRecursive(featureID, prevState, 0x1, newScores);
          break;
        case Moses::GHKM::PhraseOrientation::REO_CLASS_RIGHT:
          newScores[1] += std::log(orientationPhraseProperty->GetLeftToRightProbabilitySwap());
          // if sub-derivation has left-boundary non-terminal:
          // add recursive actual score of boundary non-terminal from subderivation
          LeftBoundaryL2RScoreRecursive(featureID, prevState, 0x2, newScores);
          break;
        case Moses::GHKM::PhraseOrientation::REO_CLASS_DLEFT:
          newScores[2] += std::log(orientationPhraseProperty->GetLeftToRightProbabilityDiscontinuous());
          // if sub-derivation has left-boundary non-terminal:
          // add recursive actual score of boundary non-terminal from subderivation
          LeftBoundaryL2RScoreRecursive(featureID, prevState, 0x4, newScores);
          break;
        case Moses::GHKM::PhraseOrientation::REO_CLASS_DRIGHT:
          newScores[2] += std::log(orientationPhraseProperty->GetLeftToRightProbabilityDiscontinuous());
          // if sub-derivation has left-boundary non-terminal:
          // add recursive actual score of boundary non-terminal from subderivation
          LeftBoundaryL2RScoreRecursive(featureID, prevState, 0x4, newScores);
          break;
        case Moses::GHKM::PhraseOrientation::REO_CLASS_UNKNOWN:
          // modelType == Moses::GHKM::PhraseOrientation::REO_MSLR
          newScores[2] += std::log(orientationPhraseProperty->GetLeftToRightProbabilityDiscontinuous());
          // if sub-derivation has left-boundary non-terminal:
          // add recursive actual score of boundary non-terminal from subderivation
          LeftBoundaryL2RScoreRecursive(featureID, prevState, 0x4, newScores);
          break;
        default:
          UTIL_THROW2(GetScoreProducerDescription()
                      << ": Unsupported orientation type.");
          break;
        }
      }


      // RIGHT-TO-LEFT DIRECTION

      Moses::GHKM::PhraseOrientation::REO_CLASS r2lOrientation = phraseOrientation.GetOrientationInfo(sourceIndex,sourceIndex,Moses::GHKM::PhraseOrientation::REO_DIR_R2L);

      IFFEATUREVERBOSE(2) {
        FEATUREVERBOSE(2, "r2lOrientation ");
        switch (r2lOrientation) {
        case Moses::GHKM::PhraseOrientation::REO_CLASS_LEFT:
          FEATUREVERBOSE2(2, "mono" << std::endl);
          break;
        case Moses::GHKM::PhraseOrientation::REO_CLASS_RIGHT:
          FEATUREVERBOSE2(2, "swap" << std::endl);
          break;
        case Moses::GHKM::PhraseOrientation::REO_CLASS_DLEFT:
          FEATUREVERBOSE2(2, "dleft" << std::endl);
          break;
        case Moses::GHKM::PhraseOrientation::REO_CLASS_DRIGHT:
          FEATUREVERBOSE2(2, "dright" << std::endl);
          break;
        case Moses::GHKM::PhraseOrientation::REO_CLASS_UNKNOWN:
          // modelType == Moses::GHKM::PhraseOrientation::REO_MSLR
          FEATUREVERBOSE2(2, "unknown->dleft" << std::endl);
          break;
        default:
          UTIL_THROW2(GetScoreProducerDescription()
                      << ": Unsupported orientation type.");
          break;
        }
      }

      bool delayedScoringR2L = false;

      if ( ((targetIndex == currTarPhr.GetSize()-1) || !phraseOrientation.TargetSpanIsAligned(targetIndex,currTarPhr.GetSize()-1)) // boundary non-terminal in rule-final position (right boundary)
           && (currTarPhrLHS != m_glueTargetLHS) ) { // and not glue rule
        // delay right-to-left scoring

        FEATUREVERBOSE(3, "Right boundary");
        if (targetIndex != currTarPhr.GetSize()-1) {
          FEATUREVERBOSE2(3, " (with targetIndex!=currTarPhr.GetSize()-1)");
        }
        FEATUREVERBOSE2(3, std::endl);

        bool previousSourceSpanIsAligned  = ( (sourceIndex > 0) && phraseOrientation.SourceSpanIsAligned(0,sourceIndex-1) );
        bool followingSourceSpanIsAligned = ( (sourceIndex < currSrcPhr->GetSize()-1) && phraseOrientation.SourceSpanIsAligned(sourceIndex,currSrcPhr->GetSize()-1) );

        FEATUREVERBOSE(4, "previousSourceSpanIsAligned = " << previousSourceSpanIsAligned << std::endl);
        FEATUREVERBOSE(4, "followingSourceSpanIsAligned = " << followingSourceSpanIsAligned << std::endl;);

        if (previousSourceSpanIsAligned && followingSourceSpanIsAligned) {
          // discontinuous
          r2lOrientation = Moses::GHKM::PhraseOrientation::REO_CLASS_DLEFT;
        } else {
          FEATUREVERBOSE(3, "Delaying right-to-left scoring" << std::endl);

          delayedScoringR2L = true;
          std::bitset<3> possibleFutureOrientationsR2L(0x7);
          possibleFutureOrientationsR2L[0] = !followingSourceSpanIsAligned;
          possibleFutureOrientationsR2L[1] = !previousSourceSpanIsAligned;

          // add heuristic scores

          std::vector<float> weightsVector = StaticData::Instance().GetAllWeights().GetScoresForProducer(this);
          std::vector<float> scoresR2L;
          scoresR2L.push_back( std::log(orientationPhraseProperty->GetRightToLeftProbabilityMono()) );
          scoresR2L.push_back( std::log(orientationPhraseProperty->GetRightToLeftProbabilitySwap()) );
          scoresR2L.push_back( std::log(orientationPhraseProperty->GetRightToLeftProbabilityDiscontinuous()) );
          std::vector<float> weightedScoresR2L;
          for ( size_t i=0; i<3; ++i ) {
            weightedScoresR2L.push_back( weightsVector[m_offsetR2LScores+i] * scoresR2L[i] );
          }

          size_t heuristicScoreIndex = 0;
          for (size_t i=1; i<3; ++i) {
            if (possibleFutureOrientationsR2L[i]) {
              if (weightedScoresR2L[i] > weightedScoresR2L[heuristicScoreIndex]) {
                heuristicScoreIndex = i;
              }
            }
          }

          IFFEATUREVERBOSE(5) {
            FEATUREVERBOSE(5, "Heuristic score computation (R2L): "
                           << "heuristicScoreIndex= " << heuristicScoreIndex);
            for (size_t i=0; i<3; ++i)
              FEATUREVERBOSE2(5, " weightsVector[" << m_offsetR2LScores+i << "]= " << weightsVector[m_offsetR2LScores+i]);
            for (size_t i=0; i<3; ++i)
              FEATUREVERBOSE2(5, " scoresR2L[" << i << "]= " << scoresR2L[i]);
            for (size_t i=0; i<3; ++i)
              FEATUREVERBOSE2(5, " weightedScoresR2L[" << i << "]= " << weightedScoresR2L[i]);
            for (size_t i=0; i<3; ++i)
              FEATUREVERBOSE2(5, " possibleFutureOrientationsR2L[" << i << "]= " << possibleFutureOrientationsR2L[i]);
            if ( possibleFutureOrientationsR2L == 0x7 ) {
              FEATUREVERBOSE2(5, " (all orientations possible)");
            }
            FEATUREVERBOSE2(5, std::endl);
          }

          newScores[m_offsetR2LScores+heuristicScoreIndex] += scoresR2L[heuristicScoreIndex];
          state->SetRightBoundaryR2L(scoresR2L, heuristicScoreIndex, possibleFutureOrientationsR2L, prevState);

          if ( (possibleFutureOrientationsR2L & prevState->m_rightBoundaryNonTerminalR2LPossibleFutureOrientations) == 0x4 ) {
            // recursive: discontinuous orientation
            FEATUREVERBOSE(5, "previous state: R2L discontinuous orientation "
                           << possibleFutureOrientationsR2L << " & " << prevState->m_rightBoundaryNonTerminalR2LPossibleFutureOrientations
                           << " = " << (possibleFutureOrientationsR2L & prevState->m_rightBoundaryNonTerminalR2LPossibleFutureOrientations)
                           << std::endl);
            RightBoundaryR2LScoreRecursive(featureID, prevState, 0x4, newScores);
            state->m_rightBoundaryRecursionGuard = true; // prevent subderivation from being scored recursively multiple times
          }
        }
      }

      if (!delayedScoringR2L) {
        switch (r2lOrientation) {
        case Moses::GHKM::PhraseOrientation::REO_CLASS_LEFT:
          newScores[m_offsetR2LScores+0] += std::log(orientationPhraseProperty->GetRightToLeftProbabilityMono());
          // if sub-derivation has right-boundary non-terminal:
          // add recursive actual score of boundary non-terminal from subderivation
          RightBoundaryR2LScoreRecursive(featureID, prevState, 0x1, newScores);
          break;
        case Moses::GHKM::PhraseOrientation::REO_CLASS_RIGHT:
          newScores[m_offsetR2LScores+1] += std::log(orientationPhraseProperty->GetRightToLeftProbabilitySwap());
          // if sub-derivation has right-boundary non-terminal:
          // add recursive actual score of boundary non-terminal from subderivation
          RightBoundaryR2LScoreRecursive(featureID, prevState, 0x2, newScores);
          break;
        case Moses::GHKM::PhraseOrientation::REO_CLASS_DLEFT:
          newScores[m_offsetR2LScores+2] += std::log(orientationPhraseProperty->GetRightToLeftProbabilityDiscontinuous());
          // if sub-derivation has right-boundary non-terminal:
          // add recursive actual score of boundary non-terminal from subderivation
          RightBoundaryR2LScoreRecursive(featureID, prevState, 0x4, newScores);
          break;
        case Moses::GHKM::PhraseOrientation::REO_CLASS_DRIGHT:
          newScores[m_offsetR2LScores+2] += std::log(orientationPhraseProperty->GetRightToLeftProbabilityDiscontinuous());
          // if sub-derivation has right-boundary non-terminal:
          // add recursive actual score of boundary non-terminal from subderivation
          RightBoundaryR2LScoreRecursive(featureID, prevState, 0x4, newScores);
          break;
        case Moses::GHKM::PhraseOrientation::REO_CLASS_UNKNOWN:
          // modelType == Moses::GHKM::PhraseOrientation::REO_MSLR
          newScores[m_offsetR2LScores+2] += std::log(orientationPhraseProperty->GetRightToLeftProbabilityDiscontinuous());
          // if sub-derivation has right-boundary non-terminal:
          // add recursive actual score of boundary non-terminal from subderivation
          RightBoundaryR2LScoreRecursive(featureID, prevState, 0x4, newScores);
          break;
        default:
          UTIL_THROW2(GetScoreProducerDescription()
                      << ": Unsupported orientation type.");
          break;
        }
      }
    } else {
      // abort with error message if the phrase does not translate an unknown word
      UTIL_THROW_IF2(!prevTarPhr.GetWord(0).IsOOV(), GetScoreProducerDescription()
                     << ": Missing Orientation property. "
                     << "Please check phrase table and glue rules.");
    }
  }

  accumulator->PlusEquals(this, newScores);

  return state;
}

void PhraseOrientationFeature::LeftBoundaryL2RScoreRecursive(int featureID,
    const PhraseOrientationFeatureState *state,
    const std::bitset<3> orientation,
    std::vector<float>& newScores) const
{
  if (state->m_leftBoundaryIsSet) {
    // subtract heuristic score from subderivation
    newScores[state->m_leftBoundaryNonTerminalL2RHeuristicScoreIndex] -= state->m_leftBoundaryNonTerminalL2RScores[state->m_leftBoundaryNonTerminalL2RHeuristicScoreIndex];

    // add actual score
    std::bitset<3> recursiveOrientation = orientation;
    if ( (orientation == 0x4) || (orientation == 0x0) ) {
      // discontinuous
      newScores[2] += state->GetLeftBoundaryL2RScoreDiscontinuous();
    } else {
      recursiveOrientation &= state->m_leftBoundaryNonTerminalL2RPossibleFutureOrientations;
      if ( recursiveOrientation == 0x1 ) {
        // monotone
        newScores[0] += state->GetLeftBoundaryL2RScoreMono();
      } else if ( recursiveOrientation == 0x2 ) {
        // swap
        newScores[1] += state->GetLeftBoundaryL2RScoreSwap();
      } else if ( recursiveOrientation == 0x4 ) {
        // discontinuous
        newScores[2] += state->GetLeftBoundaryL2RScoreDiscontinuous();
      } else if ( recursiveOrientation == 0x0 ) {
        // discontinuous
        newScores[2] += state->GetLeftBoundaryL2RScoreDiscontinuous();
      } else {
        UTIL_THROW2(GetScoreProducerDescription()
                    << ": Error in recursive scoring.");
      }
    }

    FEATUREVERBOSE(6, "Left boundary recursion: " << orientation << " & " << state->m_leftBoundaryNonTerminalL2RPossibleFutureOrientations << " = " << recursiveOrientation
                   << " --- Subtracted heuristic score: " << state->m_leftBoundaryNonTerminalL2RScores[state->m_leftBoundaryNonTerminalL2RHeuristicScoreIndex] << std::endl);

    if (!state->m_leftBoundaryRecursionGuard) {
      // recursive call
      const PhraseOrientationFeatureState* prevState = state->m_leftBoundaryPrevState;
      LeftBoundaryL2RScoreRecursive(featureID, prevState, recursiveOrientation, newScores);
    } else {
      FEATUREVERBOSE(6, "m_leftBoundaryRecursionGuard" << std::endl);
    }
  }
}

void PhraseOrientationFeature::RightBoundaryR2LScoreRecursive(int featureID,
    const PhraseOrientationFeatureState *state,
    const std::bitset<3> orientation,
    std::vector<float>& newScores) const
{
  if (state->m_rightBoundaryIsSet) {
    // subtract heuristic score from subderivation
    newScores[m_offsetR2LScores+state->m_rightBoundaryNonTerminalR2LHeuristicScoreIndex] -= state->m_rightBoundaryNonTerminalR2LScores[state->m_rightBoundaryNonTerminalR2LHeuristicScoreIndex];

    // add actual score
    std::bitset<3> recursiveOrientation = orientation;
    if ( (orientation == 0x4) || (orientation == 0x0) ) {
      // discontinuous
      newScores[m_offsetR2LScores+2] += state->GetRightBoundaryR2LScoreDiscontinuous();
    } else {
      recursiveOrientation &= state->m_rightBoundaryNonTerminalR2LPossibleFutureOrientations;
      if ( recursiveOrientation == 0x1 ) {
        // monotone
        newScores[m_offsetR2LScores+0] += state->GetRightBoundaryR2LScoreMono();
      } else if ( recursiveOrientation == 0x2 ) {
        // swap
        newScores[m_offsetR2LScores+1] += state->GetRightBoundaryR2LScoreSwap();
      } else if ( recursiveOrientation == 0x4 ) {
        // discontinuous
        newScores[m_offsetR2LScores+2] += state->GetRightBoundaryR2LScoreDiscontinuous();
      } else if ( recursiveOrientation == 0x0 ) {
        // discontinuous
        newScores[m_offsetR2LScores+2] += state->GetRightBoundaryR2LScoreDiscontinuous();
      } else {
        UTIL_THROW2(GetScoreProducerDescription()
                    << ": Error in recursive scoring.");
      }
    }

    FEATUREVERBOSE(6, "Right boundary recursion: " << orientation << " & " << state->m_rightBoundaryNonTerminalR2LPossibleFutureOrientations << " = " << recursiveOrientation
                   << " --- Subtracted heuristic score: " << state->m_rightBoundaryNonTerminalR2LScores[state->m_rightBoundaryNonTerminalR2LHeuristicScoreIndex] << std::endl);

    if (!state->m_rightBoundaryRecursionGuard) {
      // recursive call
      const PhraseOrientationFeatureState* prevState = state->m_rightBoundaryPrevState;
      RightBoundaryR2LScoreRecursive(featureID, prevState, recursiveOrientation, newScores);
    } else {
      FEATUREVERBOSE(6, "m_rightBoundaryRecursionGuard" << std::endl);
    }
  }
}


}

