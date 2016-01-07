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
#include <boost/shared_ptr.hpp>


namespace Moses
{
size_t PhraseOrientationFeatureState::hash() const
{
  if (!m_distinguishStates) {
    return 0;
  }

  size_t ret = 0;

  if (m_leftBoundaryIsSet) {
    HashCombineLeftBoundaryRecursive(ret, *this, m_useSparseNT);
  }
  if (m_rightBoundaryIsSet) {
    boost::hash_combine(ret, 42);
    HashCombineRightBoundaryRecursive(ret, *this, m_useSparseNT);
  }

  return 0;
}

bool PhraseOrientationFeatureState::operator==(const FFState& other) const
{
  if (!m_distinguishStates) {
    return true;
  }

  const PhraseOrientationFeatureState &otherState = static_cast<const PhraseOrientationFeatureState&>(other);

  if (!m_leftBoundaryIsSet && !otherState.m_leftBoundaryIsSet &&
      !m_rightBoundaryIsSet && !otherState.m_rightBoundaryIsSet) {
    return true;
  }
  if (m_leftBoundaryIsSet != otherState.m_leftBoundaryIsSet) {
    return false;
  }
  if (m_rightBoundaryIsSet != otherState.m_rightBoundaryIsSet) {
    return false;
  }

  if (m_leftBoundaryIsSet) {
    int compareLeft = CompareLeftBoundaryRecursive(*this, otherState, m_useSparseNT);
    if (compareLeft != 0) {
      return false;
    }
  }
  if (m_rightBoundaryIsSet) {
    int compareRight = CompareRightBoundaryRecursive(*this, otherState, m_useSparseNT);
    if (compareRight != 0) {
      return false;
    }
  }

  return true;
}


const std::string PhraseOrientationFeature::MORIENT("M");
const std::string PhraseOrientationFeature::SORIENT("S");
const std::string PhraseOrientationFeature::DORIENT("D");


PhraseOrientationFeature::PhraseOrientationFeature(const std::string &line)
  : StatefulFeatureFunction(6, line)
  , m_glueLabelStr("Q")
  , m_noScoreBoundary(false)
  , m_monotoneScoreBoundary(false)
  , m_distinguishStates(false)
  , m_lookaheadScore(false)
  , m_heuristicScoreUseWeights(false)
  , m_useSparseWord(false)
  , m_useSparseNT(false)
  , m_offsetR2LScores(m_numScoreComponents/2)
  , m_useTargetWordList(false)
  , m_useSourceWordList(false)
{
  VERBOSE(1, "Initializing feature " << GetScoreProducerDescription() << " ...");
  ReadParameters();
  FactorCollection &factorCollection = FactorCollection::Instance();
  m_glueLabel = factorCollection.AddFactor(m_glueLabelStr, true);
  VERBOSE(1, " Done." << std::endl);
}


void PhraseOrientationFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "glue-label") {
    m_glueLabelStr = value;
  } else if (key == "no-score-boundary") {
    m_noScoreBoundary = Scan<bool>(value);
  } else if (key == "monotone-score-boundary") {
    m_monotoneScoreBoundary = Scan<bool>(value);
  } else if (key == "distinguish-states") {
    m_distinguishStates = Scan<bool>(value);
  } else if (key == "lookahead-score") {
    m_lookaheadScore = Scan<bool>(value);
  } else if (key == "heuristic-score-use-weights") {
    m_heuristicScoreUseWeights = Scan<bool>(value);
  } else if (key == "sparse-word") {
    m_useSparseWord = Scan<bool>(value);
  } else if (key == "sparse-nt") {
    m_useSparseNT = Scan<bool>(value);
  } else if (key == "target-word-list") {
    m_filenameTargetWordList = value;
  } else if (key == "source-word-list") {
    m_filenameSourceWordList = value;
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}


void PhraseOrientationFeature::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  if ( !m_filenameTargetWordList.empty() ) {
    LoadWordList(m_filenameTargetWordList,m_targetWordList);
    m_useTargetWordList = true;
  }
  if ( !m_filenameSourceWordList.empty() ) {
    LoadWordList(m_filenameSourceWordList,m_sourceWordList);
    m_useSourceWordList = true;
  }
}


void PhraseOrientationFeature::LoadWordList(const std::string& filename,
    boost::unordered_set<const Factor*>& list)
{
  FEATUREVERBOSE(2, "Loading word list from file " << filename << std::endl);
  FactorCollection &factorCollection = FactorCollection::Instance();
  list.clear();
  std::string line;
  InputFileStream inFile(filename);

  while (getline(inFile, line)) {
    const Factor *factor = factorCollection.AddFactor(line, false);
    list.insert(factor);
  }

  inFile.Close();
}


void PhraseOrientationFeature::EvaluateInIsolation(const Phrase &source,
    const TargetPhrase &targetPhrase,
    ScoreComponentCollection &scoreBreakdown,
    ScoreComponentCollection &estimatedScores) const
{
  targetPhrase.SetRuleSource(source);
  const Factor* targetPhraseLHS = targetPhrase.GetTargetLHS()[0];

  if (const PhraseProperty *property = targetPhrase.GetProperty("Orientation")) {
    const OrientationPhraseProperty *orientationPhraseProperty = static_cast<const OrientationPhraseProperty*>(property);
    if (m_lookaheadScore) {
      LookaheadScore(orientationPhraseProperty, scoreBreakdown, targetPhraseLHS);
    }
  } else {
    // abort with error message if the phrase does not translate an unknown word
    UTIL_THROW_IF2(!targetPhrase.GetWord(0).IsOOV(), GetScoreProducerDescription()
                   << ": Missing Orientation property. "
                   << "Please check phrase table and glue rules.");
  }

  IFFEATUREVERBOSE(2) {
    FEATUREVERBOSE(2, "BEGIN========EvaluateInIsolation========" << std::endl);
    FEATUREVERBOSE(2, source << std::endl);
    FEATUREVERBOSE(2, targetPhrase << std::endl);

    for (AlignmentInfo::const_iterator it=targetPhrase.GetAlignTerm().begin();
         it!=targetPhrase.GetAlignTerm().end(); ++it) {
      FEATUREVERBOSE(2, "alignTerm " << it->first << " " << it->second << std::endl);
    }

    for (AlignmentInfo::const_iterator it=targetPhrase.GetAlignNonTerm().begin();
         it!=targetPhrase.GetAlignNonTerm().end(); ++it) {
      FEATUREVERBOSE(2, "alignNonTerm " << it->first << " " << it->second << std::endl);
    }
  }


  if (targetPhrase.GetAlignNonTerm().GetSize() != 0) {

    // Initialize phrase orientation scoring object
    MosesTraining::PhraseOrientation phraseOrientation(source.GetSize(), targetPhrase.GetSize(),
        targetPhrase.GetAlignTerm(), targetPhrase.GetAlignNonTerm());

    PhraseOrientationFeature::ReoClassData* reoClassData = new PhraseOrientationFeature::ReoClassData();

    // Determine orientation classes of non-terminals

    for (AlignmentInfo::const_iterator it=targetPhrase.GetAlignNonTerm().begin();
         it!=targetPhrase.GetAlignNonTerm().end(); ++it) {
      size_t sourceIndex = it->first;
      size_t targetIndex = it->second;

      // LEFT-TO-RIGHT DIRECTION

      MosesTraining::PhraseOrientation::REO_CLASS l2rOrientation = phraseOrientation.GetOrientationInfo(sourceIndex,sourceIndex,MosesTraining::PhraseOrientation::REO_DIR_L2R);

      if ( ((targetIndex == 0) || !phraseOrientation.TargetSpanIsAligned(0,targetIndex)) // boundary non-terminal in rule-initial position (left boundary)
           && (targetPhraseLHS != m_glueLabel) ) { // and not glue rule

        FEATUREVERBOSE(3, "Left boundary: targetIndex== " << targetIndex);
        if (targetIndex != 0) {
          FEATUREVERBOSE2(3, " (!=0)");
        }
        FEATUREVERBOSE2(3, std::endl);

        reoClassData->firstNonTerminalPreviousSourceSpanIsAligned = ( (sourceIndex > 0) && phraseOrientation.SourceSpanIsAligned(0,sourceIndex-1) );
        reoClassData->firstNonTerminalFollowingSourceSpanIsAligned = ( (sourceIndex < source.GetSize()-1) && phraseOrientation.SourceSpanIsAligned(sourceIndex,source.GetSize()-1) );

        FEATUREVERBOSE(4, "firstNonTerminalPreviousSourceSpanIsAligned== " << reoClassData->firstNonTerminalPreviousSourceSpanIsAligned << std::endl);
        FEATUREVERBOSE(4, "firstNonTerminalFollowingSourceSpanIsAligned== " << reoClassData->firstNonTerminalFollowingSourceSpanIsAligned << std::endl;);

        if (reoClassData->firstNonTerminalPreviousSourceSpanIsAligned &&
            reoClassData->firstNonTerminalFollowingSourceSpanIsAligned) {
          // discontinuous
          l2rOrientation = MosesTraining::PhraseOrientation::REO_CLASS_DLEFT;
        } else {
          reoClassData->firstNonTerminalIsBoundary = true;
        }
      }

      reoClassData->nonTerminalReoClassL2R.push_back(l2rOrientation);

      // RIGHT-TO-LEFT DIRECTION

      MosesTraining::PhraseOrientation::REO_CLASS r2lOrientation = phraseOrientation.GetOrientationInfo(sourceIndex,sourceIndex,MosesTraining::PhraseOrientation::REO_DIR_R2L);

      if ( ((targetIndex == targetPhrase.GetSize()-1) || !phraseOrientation.TargetSpanIsAligned(targetIndex,targetPhrase.GetSize()-1)) // boundary non-terminal in rule-final position (right boundary)
           && (targetPhraseLHS != m_glueLabel) ) { // and not glue rule

        FEATUREVERBOSE(3, "Right boundary: targetIndex== " << targetIndex);
        if (targetIndex != targetPhrase.GetSize()-1) {
          FEATUREVERBOSE2(3, " (!=targetPhrase.GetSize()-1)");
        }
        FEATUREVERBOSE2(3, std::endl);

        reoClassData->lastNonTerminalPreviousSourceSpanIsAligned = ( (sourceIndex > 0) && phraseOrientation.SourceSpanIsAligned(0,sourceIndex-1) );
        reoClassData->lastNonTerminalFollowingSourceSpanIsAligned = ( (sourceIndex < source.GetSize()-1) && phraseOrientation.SourceSpanIsAligned(sourceIndex,source.GetSize()-1) );

        FEATUREVERBOSE(4, "lastNonTerminalPreviousSourceSpanIsAligned== " << reoClassData->lastNonTerminalPreviousSourceSpanIsAligned << std::endl);
        FEATUREVERBOSE(4, "lastNonTerminalFollowingSourceSpanIsAligned== " << reoClassData->lastNonTerminalFollowingSourceSpanIsAligned << std::endl;);

        if (reoClassData->lastNonTerminalPreviousSourceSpanIsAligned &&
            reoClassData->lastNonTerminalFollowingSourceSpanIsAligned) {
          // discontinuous
          r2lOrientation = MosesTraining::PhraseOrientation::REO_CLASS_DLEFT;
        } else {
          reoClassData->lastNonTerminalIsBoundary = true;
        }
      }

      reoClassData->nonTerminalReoClassR2L.push_back(r2lOrientation);
    }

    bool inserted = targetPhrase.SetData("Orientation", boost::shared_ptr<void>(reoClassData));
    UTIL_THROW_IF2(!inserted, GetScoreProducerDescription()
                   << ": Insertion of orientation data attempted repeatedly. ");
  }
  FEATUREVERBOSE(2, "END========EvaluateInIsolation========" << std::endl);
}


FFState* PhraseOrientationFeature::EvaluateWhenApplied(
  const ChartHypothesis& hypo,
  int featureID, // used to index the state in the previous hypotheses
  ScoreComponentCollection* accumulator) const
{
  // Dense scores
  std::vector<float> newScores(m_numScoreComponents,0);

  // Read Orientation property
  const TargetPhrase &currTarPhr = hypo.GetCurrTargetPhrase();
  const Phrase *currSrcPhr = currTarPhr.GetRuleSource();

  // State: used to propagate orientation probabilities in case of boundary non-terminals
  PhraseOrientationFeatureState *state = new PhraseOrientationFeatureState(m_distinguishStates,m_useSparseWord,m_useSparseNT);

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

  // Retrieve phrase orientation scoring object
  const PhraseOrientationFeature::ReoClassData *reoClassData = NULL;
  if (currTarPhr.GetAlignNonTerm().GetSize() != 0) {
    const boost::shared_ptr<void> data = currTarPhr.GetData("Orientation");
    UTIL_THROW_IF2(!data, GetScoreProducerDescription()
                   << ": Orientation data not set in target phrase. ");
    reoClassData = static_cast<const PhraseOrientationFeature::ReoClassData*>( data.get() );
  }

  // Get index map for underlying hypotheses
  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
    currTarPhr.GetAlignNonTerm().GetNonTermIndexMap();

  // Retrieve non-terminals orientation classes & score orientations

  size_t nNT = 0;

  for (AlignmentInfo::const_iterator it=currTarPhr.GetAlignNonTerm().begin();
       it!=currTarPhr.GetAlignNonTerm().end(); ++it) {
    size_t sourceIndex = it->first;
    size_t targetIndex = it->second;
    size_t nonTermIndex = nonTermIndexMap[targetIndex];

    FEATUREVERBOSE(2, "Scoring nonTermIndex== " << nonTermIndex << " targetIndex== " << targetIndex << " sourceIndex== " << sourceIndex << std::endl);

    // consult subderivation
    const ChartHypothesis *prevHypo = hypo.GetPrevHypo(nonTermIndex);
    const TargetPhrase &prevTarPhr = prevHypo->GetCurrTargetPhrase();
    const Factor* prevTarPhrLHS = prevTarPhr.GetTargetLHS()[0];

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

      if (m_lookaheadScore) {
        LookaheadScore(orientationPhraseProperty, *accumulator, prevTarPhrLHS, true);
      }

      const PhraseOrientationFeatureState* prevState =
        static_cast<const PhraseOrientationFeatureState*>(prevHypo->GetFFState(featureID));


      // LEFT-TO-RIGHT DIRECTION

      MosesTraining::PhraseOrientation::REO_CLASS l2rOrientation = reoClassData->nonTerminalReoClassL2R[nNT];

      IFFEATUREVERBOSE(2) {
        FEATUREVERBOSE(2, "l2rOrientation ");
        switch (l2rOrientation) {
        case MosesTraining::PhraseOrientation::REO_CLASS_LEFT:
          FEATUREVERBOSE2(2, "mono" << std::endl);
          break;
        case MosesTraining::PhraseOrientation::REO_CLASS_RIGHT:
          FEATUREVERBOSE2(2, "swap" << std::endl);
          break;
        case MosesTraining::PhraseOrientation::REO_CLASS_DLEFT:
          FEATUREVERBOSE2(2, "dleft" << std::endl);
          break;
        case MosesTraining::PhraseOrientation::REO_CLASS_DRIGHT:
          FEATUREVERBOSE2(2, "dright" << std::endl);
          break;
        case MosesTraining::PhraseOrientation::REO_CLASS_UNKNOWN:
          // modelType == MosesTraining::PhraseOrientation::REO_MSLR
          FEATUREVERBOSE2(2, "unknown->dleft" << std::endl);
          break;
        default:
          UTIL_THROW2(GetScoreProducerDescription()
                      << ": Unsupported orientation type.");
          break;
        }
      }

      if ( (nNT == 0) && reoClassData->firstNonTerminalIsBoundary && !m_monotoneScoreBoundary ) {

        if (!m_noScoreBoundary) {
          // delay left-to-right scoring

          FEATUREVERBOSE(3, "Delaying left-to-right scoring" << std::endl);

          std::bitset<3> possibleFutureOrientationsL2R(0x7);
          possibleFutureOrientationsL2R[0] = !reoClassData->firstNonTerminalPreviousSourceSpanIsAligned;
          possibleFutureOrientationsL2R[1] = !reoClassData->firstNonTerminalFollowingSourceSpanIsAligned;

          // add heuristic scores

          std::vector<float> scoresL2R;
          scoresL2R.push_back( TransformScore(orientationPhraseProperty->GetLeftToRightProbabilityMono()) );
          scoresL2R.push_back( TransformScore(orientationPhraseProperty->GetLeftToRightProbabilitySwap()) );
          scoresL2R.push_back( TransformScore(orientationPhraseProperty->GetLeftToRightProbabilityDiscontinuous()) );

          size_t heuristicScoreIndexL2R = GetHeuristicScoreIndex(scoresL2R, 0, possibleFutureOrientationsL2R);

          newScores[heuristicScoreIndexL2R] += scoresL2R[heuristicScoreIndexL2R];
          state->SetLeftBoundaryL2R(scoresL2R, heuristicScoreIndexL2R, possibleFutureOrientationsL2R, prevTarPhrLHS, prevState);

          if ( (possibleFutureOrientationsL2R & prevState->m_leftBoundaryNonTerminalL2RPossibleFutureOrientations) == 0x4 ) {
            // recursive: discontinuous orientation
            FEATUREVERBOSE(5, "previous state: L2R discontinuous orientation "
                           << possibleFutureOrientationsL2R << " & " << prevState->m_leftBoundaryNonTerminalL2RPossibleFutureOrientations
                           << " = " << (possibleFutureOrientationsL2R & prevState->m_leftBoundaryNonTerminalL2RPossibleFutureOrientations)
                           << std::endl);
            LeftBoundaryL2RScoreRecursive(featureID, prevState, 0x4, newScores, accumulator);
            state->m_leftBoundaryRecursionGuard = true; // prevent subderivation from being scored recursively multiple times
          }
        }

      } else {

        if ( l2rOrientation == MosesTraining::PhraseOrientation::REO_CLASS_LEFT ) {

          newScores[0] += TransformScore(orientationPhraseProperty->GetLeftToRightProbabilityMono());

          if (!m_noScoreBoundary && !m_monotoneScoreBoundary) {
            // if sub-derivation has left-boundary non-terminal:
            // add recursive actual score of boundary non-terminal from subderivation
            LeftBoundaryL2RScoreRecursive(featureID, prevState, 0x1, newScores, accumulator);
          }

        } else if ( l2rOrientation == MosesTraining::PhraseOrientation::REO_CLASS_RIGHT ) {

          newScores[1] += TransformScore(orientationPhraseProperty->GetLeftToRightProbabilitySwap());

          if (!m_noScoreBoundary && !m_monotoneScoreBoundary) {
            // if sub-derivation has left-boundary non-terminal:
            // add recursive actual score of boundary non-terminal from subderivation
            LeftBoundaryL2RScoreRecursive(featureID, prevState, 0x2, newScores, accumulator);
          }

        } else if ( ( l2rOrientation == MosesTraining::PhraseOrientation::REO_CLASS_DLEFT ) ||
                    ( l2rOrientation == MosesTraining::PhraseOrientation::REO_CLASS_DRIGHT ) ||
                    ( l2rOrientation == MosesTraining::PhraseOrientation::REO_CLASS_UNKNOWN ) ) {

          newScores[2] += TransformScore(orientationPhraseProperty->GetLeftToRightProbabilityDiscontinuous());

          if (!m_noScoreBoundary && !m_monotoneScoreBoundary) {
            // if sub-derivation has left-boundary non-terminal:
            // add recursive actual score of boundary non-terminal from subderivation
            LeftBoundaryL2RScoreRecursive(featureID, prevState, 0x4, newScores, accumulator);
          }

        } else {

          UTIL_THROW2(GetScoreProducerDescription()
                      << ": Unsupported orientation type.");
        }

        // sparse scores
        if ( m_useSparseWord ) {
          SparseWordL2RScore(prevHypo,accumulator,ToString(l2rOrientation));
        }
        if ( m_useSparseNT ) {
          SparseNonTerminalL2RScore(prevTarPhrLHS,accumulator,ToString(l2rOrientation));
        }
      }


      // RIGHT-TO-LEFT DIRECTION

      MosesTraining::PhraseOrientation::REO_CLASS r2lOrientation = reoClassData->nonTerminalReoClassR2L[nNT];

      IFFEATUREVERBOSE(2) {
        FEATUREVERBOSE(2, "r2lOrientation ");
        switch (r2lOrientation) {
        case MosesTraining::PhraseOrientation::REO_CLASS_LEFT:
          FEATUREVERBOSE2(2, "mono" << std::endl);
          break;
        case MosesTraining::PhraseOrientation::REO_CLASS_RIGHT:
          FEATUREVERBOSE2(2, "swap" << std::endl);
          break;
        case MosesTraining::PhraseOrientation::REO_CLASS_DLEFT:
          FEATUREVERBOSE2(2, "dleft" << std::endl);
          break;
        case MosesTraining::PhraseOrientation::REO_CLASS_DRIGHT:
          FEATUREVERBOSE2(2, "dright" << std::endl);
          break;
        case MosesTraining::PhraseOrientation::REO_CLASS_UNKNOWN:
          // modelType == MosesTraining::PhraseOrientation::REO_MSLR
          FEATUREVERBOSE2(2, "unknown->dleft" << std::endl);
          break;
        default:
          UTIL_THROW2(GetScoreProducerDescription()
                      << ": Unsupported orientation type.");
          break;
        }
      }

      if ( (nNT == currTarPhr.GetAlignNonTerm().GetSize()-1) && reoClassData->lastNonTerminalIsBoundary && !m_monotoneScoreBoundary ) {

        if (!m_noScoreBoundary) {
          // delay right-to-left scoring

          FEATUREVERBOSE(3, "Delaying right-to-left scoring" << std::endl);

          std::bitset<3> possibleFutureOrientationsR2L(0x7);
          possibleFutureOrientationsR2L[0] = !reoClassData->lastNonTerminalFollowingSourceSpanIsAligned;
          possibleFutureOrientationsR2L[1] = !reoClassData->lastNonTerminalPreviousSourceSpanIsAligned;

          // add heuristic scores

          std::vector<float> scoresR2L;
          scoresR2L.push_back( TransformScore(orientationPhraseProperty->GetRightToLeftProbabilityMono()) );
          scoresR2L.push_back( TransformScore(orientationPhraseProperty->GetRightToLeftProbabilitySwap()) );
          scoresR2L.push_back( TransformScore(orientationPhraseProperty->GetRightToLeftProbabilityDiscontinuous()) );

          size_t heuristicScoreIndexR2L = GetHeuristicScoreIndex(scoresR2L, m_offsetR2LScores, possibleFutureOrientationsR2L);

          newScores[m_offsetR2LScores+heuristicScoreIndexR2L] += scoresR2L[heuristicScoreIndexR2L];
          state->SetRightBoundaryR2L(scoresR2L, heuristicScoreIndexR2L, possibleFutureOrientationsR2L, prevTarPhrLHS, prevState);

          if ( (possibleFutureOrientationsR2L & prevState->m_rightBoundaryNonTerminalR2LPossibleFutureOrientations) == 0x4 ) {
            // recursive: discontinuous orientation
            FEATUREVERBOSE(5, "previous state: R2L discontinuous orientation "
                           << possibleFutureOrientationsR2L << " & " << prevState->m_rightBoundaryNonTerminalR2LPossibleFutureOrientations
                           << " = " << (possibleFutureOrientationsR2L & prevState->m_rightBoundaryNonTerminalR2LPossibleFutureOrientations)
                           << std::endl);
            RightBoundaryR2LScoreRecursive(featureID, prevState, 0x4, newScores, accumulator);
            state->m_rightBoundaryRecursionGuard = true; // prevent subderivation from being scored recursively multiple times
          }
        }

      } else {

        if ( r2lOrientation == MosesTraining::PhraseOrientation::REO_CLASS_LEFT ) {

          newScores[m_offsetR2LScores+0] += TransformScore(orientationPhraseProperty->GetRightToLeftProbabilityMono());

          if (!m_noScoreBoundary && !m_monotoneScoreBoundary) {
            // if sub-derivation has right-boundary non-terminal:
            // add recursive actual score of boundary non-terminal from subderivation
            RightBoundaryR2LScoreRecursive(featureID, prevState, 0x1, newScores, accumulator);
          }

        } else if ( r2lOrientation == MosesTraining::PhraseOrientation::REO_CLASS_RIGHT ) {

          newScores[m_offsetR2LScores+1] += TransformScore(orientationPhraseProperty->GetRightToLeftProbabilitySwap());

          if (!m_noScoreBoundary && !m_monotoneScoreBoundary) {
            // if sub-derivation has right-boundary non-terminal:
            // add recursive actual score of boundary non-terminal from subderivation
            RightBoundaryR2LScoreRecursive(featureID, prevState, 0x2, newScores, accumulator);
          }

        } else if ( ( r2lOrientation == MosesTraining::PhraseOrientation::REO_CLASS_DLEFT ) ||
                    ( r2lOrientation == MosesTraining::PhraseOrientation::REO_CLASS_DRIGHT ) ||
                    ( r2lOrientation == MosesTraining::PhraseOrientation::REO_CLASS_UNKNOWN ) ) {

          newScores[m_offsetR2LScores+2] += TransformScore(orientationPhraseProperty->GetRightToLeftProbabilityDiscontinuous());

          if (!m_noScoreBoundary && !m_monotoneScoreBoundary) {
            // if sub-derivation has right-boundary non-terminal:
            // add recursive actual score of boundary non-terminal from subderivation
            RightBoundaryR2LScoreRecursive(featureID, prevState, 0x4, newScores, accumulator);
          }

        } else {

          UTIL_THROW2(GetScoreProducerDescription()
                      << ": Unsupported orientation type.");
        }

        // sparse scores
        if ( m_useSparseWord ) {
          SparseWordR2LScore(prevHypo,accumulator,ToString(r2lOrientation));
        }
        if ( m_useSparseNT ) {
          SparseNonTerminalR2LScore(prevTarPhrLHS,accumulator,ToString(r2lOrientation));
        }
      }

    } else {
      // abort with error message if the phrase does not translate an unknown word
      UTIL_THROW_IF2(!prevTarPhr.GetWord(0).IsOOV(), GetScoreProducerDescription()
                     << ": Missing Orientation property. "
                     << "Please check phrase table and glue rules.");
    }

    ++nNT;
  }

  accumulator->PlusEquals(this, newScores);

  return state;
}


void PhraseOrientationFeature::LookaheadScore(const OrientationPhraseProperty *orientationPhraseProperty,
    ScoreComponentCollection &scoreBreakdown,
    const Factor* targetPhraseLHS,
    bool subtract) const
{
  size_t ffScoreIndex = m_index;

  std::vector<float> scoresL2R;
  scoresL2R.push_back( TransformScore(orientationPhraseProperty->GetLeftToRightProbabilityMono()) );
  scoresL2R.push_back( TransformScore(orientationPhraseProperty->GetLeftToRightProbabilitySwap()) );
  scoresL2R.push_back( TransformScore(orientationPhraseProperty->GetLeftToRightProbabilityDiscontinuous()) );
  size_t heuristicScoreIndexL2R = 0;
  if (targetPhraseLHS != m_glueLabel) {
    heuristicScoreIndexL2R = GetHeuristicScoreIndex(scoresL2R, 0);
  }

  if (subtract) {
    scoreBreakdown.PlusEquals(ffScoreIndex+heuristicScoreIndexL2R,
                              -scoresL2R[heuristicScoreIndexL2R]);
  } else {
    scoreBreakdown.PlusEquals(ffScoreIndex+heuristicScoreIndexL2R,
                              scoresL2R[heuristicScoreIndexL2R]);
  }

  std::vector<float> scoresR2L;
  scoresR2L.push_back( TransformScore(orientationPhraseProperty->GetRightToLeftProbabilityMono()) );
  scoresR2L.push_back( TransformScore(orientationPhraseProperty->GetRightToLeftProbabilitySwap()) );
  scoresR2L.push_back( TransformScore(orientationPhraseProperty->GetRightToLeftProbabilityDiscontinuous()) );
  size_t heuristicScoreIndexR2L = 0;
  if (targetPhraseLHS != m_glueLabel) {
    heuristicScoreIndexR2L = GetHeuristicScoreIndex(scoresR2L, m_offsetR2LScores);
  }

  if (subtract) {
    scoreBreakdown.PlusEquals(ffScoreIndex+m_offsetR2LScores+heuristicScoreIndexR2L,
                              -scoresR2L[heuristicScoreIndexR2L]);
  } else {
    scoreBreakdown.PlusEquals(ffScoreIndex+m_offsetR2LScores+heuristicScoreIndexR2L,
                              scoresR2L[heuristicScoreIndexR2L]);
  }
}


size_t PhraseOrientationFeature::GetHeuristicScoreIndex(const std::vector<float>& scores,
    size_t weightsVectorOffset,
    const std::bitset<3> possibleFutureOrientations) const
{
  std::vector<float> weightedScores;
  if (m_heuristicScoreUseWeights) {
    if (m_weightsVector.empty()) {
      m_weightsVector = StaticData::Instance().GetAllWeights().GetScoresForProducer(this);
    }
    for ( size_t i=0; i<3; ++i ) {
      weightedScores.push_back( m_weightsVector[weightsVectorOffset+i] * scores[i] );
    }
  }

  size_t heuristicScoreIndex = 0;
  for (size_t i=1; i<3; ++i) {
    if (possibleFutureOrientations[i]) {
      if (m_heuristicScoreUseWeights) {
        if (weightedScores[i] > weightedScores[heuristicScoreIndex]) {
          heuristicScoreIndex = i;
        }
      } else {
        if (scores[i] > scores[heuristicScoreIndex]) {
          heuristicScoreIndex = i;
        }
      }
    }
  }

  IFFEATUREVERBOSE(5) {
    FEATUREVERBOSE(5, "Heuristic score computation: "
                   << "heuristicScoreIndex== " << heuristicScoreIndex);
    for (size_t i=0; i<3; ++i)
      FEATUREVERBOSE2(5, " scores[" << i << "]== " << scores[i]);
    if (m_heuristicScoreUseWeights) {
      for (size_t i=0; i<3; ++i)
        FEATUREVERBOSE2(5, " m_weightsVector[" << weightsVectorOffset+i << "]== " << m_weightsVector[weightsVectorOffset+i]);
      for (size_t i=0; i<3; ++i)
        FEATUREVERBOSE2(5, " weightedScores[" << i << "]== " << weightedScores[i]);
    }
    for (size_t i=0; i<3; ++i)
      FEATUREVERBOSE2(5, " possibleFutureOrientations[" << i << "]== " << possibleFutureOrientations[i]);
    if ( possibleFutureOrientations == 0x7 ) {
      FEATUREVERBOSE2(5, " (all orientations possible)");
    }
    FEATUREVERBOSE2(5, std::endl);
  }

  return heuristicScoreIndex;
}


void PhraseOrientationFeature::LeftBoundaryL2RScoreRecursive(int featureID,
    const PhraseOrientationFeatureState *state,
    const std::bitset<3> orientation,
    std::vector<float>& newScores,
    ScoreComponentCollection* scoreBreakdown) const
// TODO: passing both newScores and scoreBreakdown seems redundant (scoreBreakdown needed for sparse scores)
{
  if (state->m_leftBoundaryIsSet) {
    const std::string* recursiveOrientationString;

    // subtract heuristic score from subderivation
    newScores[state->m_leftBoundaryNonTerminalL2RHeuristicScoreIndex] -= state->m_leftBoundaryNonTerminalL2RScores[state->m_leftBoundaryNonTerminalL2RHeuristicScoreIndex];

    // add actual score
    std::bitset<3> recursiveOrientation = orientation;
    if ( (orientation == 0x4) || (orientation == 0x0) ) {
      // discontinuous
      recursiveOrientationString = &DORIENT;
      newScores[2] += state->GetLeftBoundaryL2RScoreDiscontinuous();
    } else {
      recursiveOrientation &= state->m_leftBoundaryNonTerminalL2RPossibleFutureOrientations;
      if ( recursiveOrientation == 0x1 ) {
        // monotone
        recursiveOrientationString = &MORIENT;
        newScores[0] += state->GetLeftBoundaryL2RScoreMono();
      } else if ( recursiveOrientation == 0x2 ) {
        // swap
        recursiveOrientationString = &SORIENT;
        newScores[1] += state->GetLeftBoundaryL2RScoreSwap();
      } else if ( recursiveOrientation == 0x4 ) {
        // discontinuous
        recursiveOrientationString = &DORIENT;
        newScores[2] += state->GetLeftBoundaryL2RScoreDiscontinuous();
      } else if ( recursiveOrientation == 0x0 ) {
        // discontinuous
        recursiveOrientationString = &DORIENT;
        newScores[2] += state->GetLeftBoundaryL2RScoreDiscontinuous();
      } else {
        UTIL_THROW2(GetScoreProducerDescription()
                    << ": Error in recursive scoring.");
      }
    }

    if ( m_useSparseNT ) {
      SparseNonTerminalL2RScore(state->m_leftBoundaryNonTerminalSymbol,scoreBreakdown,recursiveOrientationString);
    }

    FEATUREVERBOSE(6, "Left boundary recursion: " << orientation << " & " << state->m_leftBoundaryNonTerminalL2RPossibleFutureOrientations << " = " << recursiveOrientation
                   << " --- Subtracted heuristic score: " << state->m_leftBoundaryNonTerminalL2RScores[state->m_leftBoundaryNonTerminalL2RHeuristicScoreIndex] << std::endl);

    if (!state->m_leftBoundaryRecursionGuard) {
      // recursive call
      const PhraseOrientationFeatureState* prevState = state->m_leftBoundaryPrevState;
      LeftBoundaryL2RScoreRecursive(featureID, prevState, recursiveOrientation, newScores, scoreBreakdown);
    } else {
      FEATUREVERBOSE(6, "m_leftBoundaryRecursionGuard" << std::endl);
    }
  }
}


void PhraseOrientationFeature::RightBoundaryR2LScoreRecursive(int featureID,
    const PhraseOrientationFeatureState *state,
    const std::bitset<3> orientation,
    std::vector<float>& newScores,
    ScoreComponentCollection* scoreBreakdown) const
// TODO: passing both newScores and scoreBreakdown seems redundant (scoreBreakdown needed for sparse scores)
{
  if (state->m_rightBoundaryIsSet) {
    const std::string* recursiveOrientationString;

    // subtract heuristic score from subderivation
    newScores[m_offsetR2LScores+state->m_rightBoundaryNonTerminalR2LHeuristicScoreIndex] -= state->m_rightBoundaryNonTerminalR2LScores[state->m_rightBoundaryNonTerminalR2LHeuristicScoreIndex];

    // add actual score
    std::bitset<3> recursiveOrientation = orientation;
    if ( (orientation == 0x4) || (orientation == 0x0) ) {
      // discontinuous
      recursiveOrientationString = &DORIENT;
      newScores[m_offsetR2LScores+2] += state->GetRightBoundaryR2LScoreDiscontinuous();
    } else {
      recursiveOrientation &= state->m_rightBoundaryNonTerminalR2LPossibleFutureOrientations;
      if ( recursiveOrientation == 0x1 ) {
        // monotone
        recursiveOrientationString = &MORIENT;
        newScores[m_offsetR2LScores+0] += state->GetRightBoundaryR2LScoreMono();
      } else if ( recursiveOrientation == 0x2 ) {
        // swap
        recursiveOrientationString = &SORIENT;
        newScores[m_offsetR2LScores+1] += state->GetRightBoundaryR2LScoreSwap();
      } else if ( recursiveOrientation == 0x4 ) {
        // discontinuous
        recursiveOrientationString = &DORIENT;
        newScores[m_offsetR2LScores+2] += state->GetRightBoundaryR2LScoreDiscontinuous();
      } else if ( recursiveOrientation == 0x0 ) {
        // discontinuous
        recursiveOrientationString = &DORIENT;
        newScores[m_offsetR2LScores+2] += state->GetRightBoundaryR2LScoreDiscontinuous();
      } else {
        UTIL_THROW2(GetScoreProducerDescription()
                    << ": Error in recursive scoring.");
      }
    }

    if ( m_useSparseNT ) {
      SparseNonTerminalR2LScore(state->m_rightBoundaryNonTerminalSymbol,scoreBreakdown,recursiveOrientationString);
    }

    FEATUREVERBOSE(6, "Right boundary recursion: " << orientation << " & " << state->m_rightBoundaryNonTerminalR2LPossibleFutureOrientations << " = " << recursiveOrientation
                   << " --- Subtracted heuristic score: " << state->m_rightBoundaryNonTerminalR2LScores[state->m_rightBoundaryNonTerminalR2LHeuristicScoreIndex] << std::endl);

    if (!state->m_rightBoundaryRecursionGuard) {
      // recursive call
      const PhraseOrientationFeatureState* prevState = state->m_rightBoundaryPrevState;
      RightBoundaryR2LScoreRecursive(featureID, prevState, recursiveOrientation, newScores, scoreBreakdown);
    } else {
      FEATUREVERBOSE(6, "m_rightBoundaryRecursionGuard" << std::endl);
    }
  }
}


void PhraseOrientationFeature::SparseWordL2RScore(const ChartHypothesis* hypo,
    ScoreComponentCollection* scoreBreakdown,
    const std::string* o) const
{
  // target word

  const ChartHypothesis* currHypo = hypo;
  const TargetPhrase* targetPhrase = &currHypo->GetCurrTargetPhrase();
  const Word* targetWord = &targetPhrase->GetWord(0);

  // TODO: boundary words in the feature state?
  while ( targetWord->IsNonTerminal() ) {
    const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
      targetPhrase->GetAlignNonTerm().GetNonTermIndexMap();
    size_t nonTermIndex = nonTermIndexMap[0];
    currHypo = currHypo->GetPrevHypo(nonTermIndex);
    targetPhrase = &currHypo->GetCurrTargetPhrase();
    targetWord = &targetPhrase->GetWord(0);
  }

  const std::string& targetWordString = (*targetWord)[0]->GetString().as_string();
  if (targetWordString != "<s>" && targetWordString != "</s>") {
    if ( !m_useTargetWordList || m_targetWordList.find((*targetWord)[0]) != m_targetWordList.end() ) {
      scoreBreakdown->PlusEquals(this,
                                 "L2R"+*o+"_tw_"+targetWordString,
                                 1);
      FEATUREVERBOSE(3, "Sparse: L2R"+*o+"_tw_"+targetWordString << std::endl);
    } else {
      scoreBreakdown->PlusEquals(this,
                                 "L2R"+*o+"_tw_OTHER",
                                 1);
      FEATUREVERBOSE(3, "Sparse: L2R"+*o+"_tw_OTHER" << std::endl);
    }
  }

  // source word

  Range sourceSpan = hypo->GetCurrSourceRange();
  const InputType& input = hypo->GetManager().GetSource();
  const Sentence& sourceSentence = static_cast<const Sentence&>(input);
  const Word& sourceWord = sourceSentence.GetWord(sourceSpan.GetStartPos());

  const std::string& sourceWordString = sourceWord[0]->GetString().as_string();
  if (sourceWordString != "<s>" && sourceWordString != "</s>") {
    if ( !m_useSourceWordList || m_sourceWordList.find(sourceWord[0]) != m_sourceWordList.end() ) {
      scoreBreakdown->PlusEquals(this,
                                 "L2R"+*o+"_sw_"+sourceWordString,
                                 1);
      FEATUREVERBOSE(3, "Sparse: L2R"+*o+"_sw_"+sourceWordString << std::endl);
    } else {
      scoreBreakdown->PlusEquals(this,
                                 "L2R"+*o+"_sw_OTHER",
                                 1);
      FEATUREVERBOSE(3, "Sparse: L2R"+*o+"_sw_OTHER" << std::endl);
    }
  }
}


void PhraseOrientationFeature::SparseWordR2LScore(const ChartHypothesis* hypo,
    ScoreComponentCollection* scoreBreakdown,
    const std::string* o) const
{
  // target word

  const ChartHypothesis* currHypo = hypo;
  const TargetPhrase* targetPhrase = &currHypo->GetCurrTargetPhrase();
  const Word* targetWord = &targetPhrase->GetWord(targetPhrase->GetSize()-1);

  // TODO: boundary words in the feature state?
  while ( targetWord->IsNonTerminal() ) {
    const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
      targetPhrase->GetAlignNonTerm().GetNonTermIndexMap();
    size_t nonTermIndex = nonTermIndexMap[targetPhrase->GetSize()-1];
    currHypo = currHypo->GetPrevHypo(nonTermIndex);
    targetPhrase = &currHypo->GetCurrTargetPhrase();
    targetWord = &targetPhrase->GetWord(targetPhrase->GetSize()-1);
  }

  const std::string& targetWordString = (*targetWord)[0]->GetString().as_string();
  if (targetWordString != "<s>" && targetWordString != "</s>") {
    if ( !m_useTargetWordList || m_targetWordList.find((*targetWord)[0]) != m_targetWordList.end() ) {
      scoreBreakdown->PlusEquals(this,
                                 "R2L"+*o+"_tw_"+targetWordString,
                                 1);
      FEATUREVERBOSE(3, "Sparse: R2L"+*o+"_tw_"+targetWordString << std::endl);
    } else {
      scoreBreakdown->PlusEquals(this,
                                 "R2L"+*o+"_tw_OTHER",
                                 1);
      FEATUREVERBOSE(3, "Sparse: R2L"+*o+"_tw_OTHER" << std::endl);
    }
  }

  // source word

  Range sourceSpan = hypo->GetCurrSourceRange();
  const InputType& input = hypo->GetManager().GetSource();
  const Sentence& sourceSentence = static_cast<const Sentence&>(input);
  const Word& sourceWord = sourceSentence.GetWord(sourceSpan.GetEndPos());

  const std::string& sourceWordString = sourceWord[0]->GetString().as_string();
  if (sourceWordString != "<s>" && sourceWordString != "</s>") {
    if ( !m_useSourceWordList || m_sourceWordList.find(sourceWord[0]) != m_sourceWordList.end() ) {
      scoreBreakdown->PlusEquals(this,
                                 "R2L"+*o+"_sw_"+sourceWordString,
                                 1);
      FEATUREVERBOSE(3, "Sparse: R2L"+*o+"_sw_"+sourceWordString << std::endl);
    } else {
      scoreBreakdown->PlusEquals(this,
                                 "R2L"+*o+"_sw_OTHER",
                                 1);
      FEATUREVERBOSE(3, "Sparse: R2L"+*o+"_sw_OTHER" << std::endl);
    }
  }
}


void PhraseOrientationFeature::SparseNonTerminalL2RScore(const Factor* nonTerminalSymbol,
    ScoreComponentCollection* scoreBreakdown,
    const std::string* o) const
{
  if ( nonTerminalSymbol != m_glueLabel ) {
    const std::string& nonTerminalString = nonTerminalSymbol->GetString().as_string();
    scoreBreakdown->PlusEquals(this,
                               "L2R"+*o+"_n_"+nonTerminalString,
                               1);
    FEATUREVERBOSE(3, "Sparse: L2R"+*o+"_n_"+nonTerminalString << std::endl);
  }
}


void PhraseOrientationFeature::SparseNonTerminalR2LScore(const Factor* nonTerminalSymbol,
    ScoreComponentCollection* scoreBreakdown,
    const std::string* o) const
{
  if ( nonTerminalSymbol != m_glueLabel ) {
    const std::string& nonTerminalString = nonTerminalSymbol->GetString().as_string();
    scoreBreakdown->PlusEquals(this,
                               "R2L"+*o+"_n_"+nonTerminalString,
                               1);
    FEATUREVERBOSE(3, "Sparse: R2L"+*o+"_n_"+nonTerminalString << std::endl);
  }
}


const std::string* PhraseOrientationFeature::ToString(const MosesTraining::PhraseOrientation::REO_CLASS o) const
{
  if ( o == MosesTraining::PhraseOrientation::REO_CLASS_LEFT ) {
    return &MORIENT;

  } else if ( o == MosesTraining::PhraseOrientation::REO_CLASS_RIGHT ) {
    return &SORIENT;

  } else if ( ( o == MosesTraining::PhraseOrientation::REO_CLASS_DLEFT ) ||
              ( o == MosesTraining::PhraseOrientation::REO_CLASS_DRIGHT ) ||
              ( o == MosesTraining::PhraseOrientation::REO_CLASS_UNKNOWN ) ) {
    return &DORIENT;

  } else {
    UTIL_THROW2(GetScoreProducerDescription()
                << ": Unsupported orientation type.");
  }
  return NULL;
}


}

