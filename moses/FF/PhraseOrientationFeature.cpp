#include <vector>
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

using namespace std;

namespace Moses
{

PhraseOrientationFeature::PhraseOrientationFeature(const std::string &line)
  : StatelessFeatureFunction(8, line)
{
  VERBOSE(1, "Initializing feature " << GetScoreProducerDescription() << " ...");
  ReadParameters();
  VERBOSE(1, " Done.");
}

void PhraseOrientationFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "tuneable") {
    m_tuneable = Scan<bool>(value);
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

void PhraseOrientationFeature::EvaluateInIsolation(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedFutureScore) const
{
  targetPhrase.SetRuleSource(source);
}


void PhraseOrientationFeature::EvaluateWhenApplied(
  const ChartHypothesis& hypo,
  ScoreComponentCollection* accumulator) const
{
  // Dense scores
  std::vector<float> newScores(m_numScoreComponents,0); // m_numScoreComponents == 8

  // Read Orientation property
  const TargetPhrase &currTarPhr = hypo.GetCurrTargetPhrase();
  const Phrase *currSrcPhr = currTarPhr.GetRuleSource();
//  const Factor* targetLHS = currTarPhr.GetTargetLHS()[0];
//  bool isGlueGrammarRule = false;

  std::map<size_t,size_t> alignMap;
  alignMap.insert(
                  currTarPhr.GetAlignTerm().begin(),
                  currTarPhr.GetAlignTerm().end());
  alignMap.insert(
                  currTarPhr.GetAlignNonTerm().begin(),
                  currTarPhr.GetAlignNonTerm().end());

  Moses::GHKM::Alignment alignment;
  std::vector<int> alignmentNTs(currTarPhr.GetSize(),-1); // TODO: can be smaller (number of right-hand side non-terminals)

  for (AlignmentInfo::const_iterator it=currTarPhr.GetAlignTerm().begin();
       it!=currTarPhr.GetAlignTerm().end(); ++it) {
    alignment.push_back(std::make_pair(it->first, it->second));
//    std::cerr << "alignTerm " << it->first << " " << it->second << std::endl;
  }

  for (AlignmentInfo::const_iterator it=currTarPhr.GetAlignNonTerm().begin();
       it!=currTarPhr.GetAlignNonTerm().end(); ++it) {
    alignment.push_back(std::make_pair(it->first, it->second));
    alignmentNTs[it->second] = it->first;
//    std::cerr << "alignNonTerm " << it->first << " " << it->second << std::endl;
  }

  // Initialize phrase orientation scoring object
  Moses::GHKM::PhraseOrientation phraseOrientation(currSrcPhr->GetSize(), currTarPhr.GetSize(), alignment);
  // TODO: Efficiency! This should be precomputed.

//  std::cerr << *currSrcPhr << std::endl;
//  std::cerr << currTarPhr << std::endl;
//  std::cerr << currSrcPhr->GetSize() << std::endl;
//  std::cerr << currTarPhr.GetSize() << std::endl;
 
  // Get index map for underlying hypotheses
  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
      currTarPhr.GetAlignNonTerm().GetNonTermIndexMap();

  // Determine & score orientations

  size_t nonTerminalNumber = 0;

  for (size_t phrasePos=0; phrasePos<currTarPhr.GetSize(); ++phrasePos) {
      // consult rule for either word or non-terminal
      const Word &word = currTarPhr.GetWord(phrasePos);
      if ( word.IsNonTerminal() ) {
          // non-terminal: consult subderivation
          size_t nonTermIndex = nonTermIndexMap[phrasePos];
          const ChartHypothesis *prevHypo = hypo.GetPrevHypo(nonTermIndex);
          const TargetPhrase &prevTarPhr = prevHypo->GetCurrTargetPhrase();
          if (const PhraseProperty *property = prevTarPhr.GetProperty("Orientation")) {
              const OrientationPhraseProperty *orientationPhraseProperty = static_cast<const OrientationPhraseProperty*>(property); 

//              std::cerr << "L2R_Mono "    << orientationPhraseProperty->GetLeftToRightProbabilityMono();
//              std::cerr << " L2R_Swap "   << orientationPhraseProperty->GetLeftToRightProbabilitySwap();
//              std::cerr << " L2R_Dright " << orientationPhraseProperty->GetLeftToRightProbabilityDright();
//              std::cerr << " L2R_Dleft "  << orientationPhraseProperty->GetLeftToRightProbabilityDleft();
//              std::cerr << " R2L_Mono "   << orientationPhraseProperty->GetRightToLeftProbabilityMono();
//              std::cerr << " R2L_Swap "   << orientationPhraseProperty->GetRightToLeftProbabilitySwap();
//              std::cerr << " R2L_Dright " << orientationPhraseProperty->GetRightToLeftProbabilityDright();
//              std::cerr << " R2L_Dleft "  << orientationPhraseProperty->GetRightToLeftProbabilityDleft();
//              std::cerr << std::endl;

              Moses::GHKM::REO_POS l2rOrientation=Moses::GHKM::UNKNOWN, r2lOrientation=Moses::GHKM::UNKNOWN;
              int sourceIndex = alignmentNTs[phrasePos];
//              std::cerr << "targetIndex " << phrasePos << " sourceIndex " << sourceIndex << std::endl;
              l2rOrientation = phraseOrientation.GetOrientationInfo(sourceIndex,sourceIndex,Moses::GHKM::L2R);
              r2lOrientation = phraseOrientation.GetOrientationInfo(sourceIndex,sourceIndex,Moses::GHKM::R2L);

//              std::cerr << "l2rOrientation ";
              switch(l2rOrientation) {
                  case Moses::GHKM::LEFT:
                      newScores[0] += std::log(orientationPhraseProperty->GetLeftToRightProbabilityMono());
//                      std::cerr << "mono" << std::endl;
                      break;
                  case Moses::GHKM::RIGHT:
                      newScores[1] += std::log(orientationPhraseProperty->GetLeftToRightProbabilitySwap());
//                      std::cerr << "swap" << std::endl;
                      break;
                  case Moses::GHKM::DRIGHT:
                      newScores[2] += std::log(orientationPhraseProperty->GetLeftToRightProbabilityDright());
//                      std::cerr << "dright" << std::endl;
                      break;
                  case Moses::GHKM::DLEFT:
                      newScores[3] += std::log(orientationPhraseProperty->GetLeftToRightProbabilityDleft());
//                      std::cerr << "dleft" << std::endl;
                      break;
                  case Moses::GHKM::UNKNOWN:
                      // modelType == Moses::GHKM::REO_MSLR
                      newScores[2] += std::log(orientationPhraseProperty->GetLeftToRightProbabilityDright());
//                      std::cerr << "unknown->dright" << std::endl;
                      break;
                  default:
                      UTIL_THROW2(GetScoreProducerDescription()
                                  << ": Unsupported orientation type.");
                      break;
              }

//              std::cerr << "r2lOrientation ";
              switch(r2lOrientation) {
                  case Moses::GHKM::LEFT:
                      newScores[4] += std::log(orientationPhraseProperty->GetRightToLeftProbabilityMono());
//                      std::cerr << "mono" << std::endl;
                      break;
                  case Moses::GHKM::RIGHT:
                      newScores[5] += std::log(orientationPhraseProperty->GetRightToLeftProbabilitySwap());
//                      std::cerr << "swap" << std::endl;
                      break;
                  case Moses::GHKM::DRIGHT:
                      newScores[6] += std::log(orientationPhraseProperty->GetRightToLeftProbabilityDright());
//                      std::cerr << "dright" << std::endl;
                      break;
                  case Moses::GHKM::DLEFT:
                      newScores[7] += std::log(orientationPhraseProperty->GetRightToLeftProbabilityDleft());
//                      std::cerr << "dleft" << std::endl;
                      break;
                  case Moses::GHKM::UNKNOWN:
                      // modelType == Moses::GHKM::REO_MSLR
                      newScores[6] += std::log(orientationPhraseProperty->GetRightToLeftProbabilityDright());
//                      std::cerr << "unknown->dright" << std::endl;
                      break;
                  default:
                      UTIL_THROW2(GetScoreProducerDescription()
                                  << ": Unsupported orientation type.");
                      break;
              }

              // TODO: Handle degenerate cases (boundary non-terminals)

          } else {
              // abort with error message if the phrase does not translate an unknown word
              UTIL_THROW_IF2(!prevTarPhr.GetWord(0).IsOOV(), GetScoreProducerDescription()
                             << ": Missing Orientation property. "
                             << "Please check phrase table and glue rules.");
          }

          ++nonTerminalNumber;
      }
  }

  accumulator->PlusEquals(this, newScores);
}

 
}

