#include <vector>
#include <limits>
#include <assert.h>
#include "PhraseOrientationFeature.h"
#include "moses/StaticData.h"
#include "moses/InputFileStream.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/Hypothesis.h"
#include "moses/ChartHypothesis.h"
#include "moses/ChartManager.h"
#include "moses/FactorCollection.h"
#include "moses/PP/OrientationPhraseProperty.h"


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


void PhraseOrientationFeature::EvaluateWhenApplied(
  const ChartHypothesis& hypo,
  ScoreComponentCollection* accumulator) const
{
  // dense scores
  std::vector<float> newScores(m_numScoreComponents,0); // m_numScoreComponents == 8

//  const InputType& input = hypo.GetManager().GetSource();

  // read Orientation property
  const TargetPhrase &currTarPhr = hypo.GetCurrTargetPhrase();
//  const Factor* targetLHS = currTarPhr.GetTargetLHS()[0];
//  bool isGlueGrammarRule = false;
  bool isUnkRule = false;
 
  // get index map for underlying hypotheses
  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
      currTarPhr.GetAlignNonTerm().GetNonTermIndexMap();

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

              std::cerr << "L2R_Mono "   << orientationPhraseProperty->GetLeftToRightProbabilityMono();
              std::cerr << " L2R_Swap "   << orientationPhraseProperty->GetLeftToRightProbabilitySwap();
              std::cerr << " L2R_Dright " << orientationPhraseProperty->GetLeftToRightProbabilityDright();
              std::cerr << " L2R_Dleft "  << orientationPhraseProperty->GetLeftToRightProbabilityDleft();
              std::cerr << " R2L_Mono "   << orientationPhraseProperty->GetRightToLeftProbabilityMono();
              std::cerr << " R2L_Swap "   << orientationPhraseProperty->GetRightToLeftProbabilitySwap();
              std::cerr << " R2L_Dright " << orientationPhraseProperty->GetRightToLeftProbabilityDright();
              std::cerr << " R2L_Dleft "  << orientationPhraseProperty->GetRightToLeftProbabilityDleft();
              std::cerr << std::endl;

          } else {
              // abort with error message if the phrase does not translate an unknown word
              UTIL_THROW_IF2(!currTarPhr.GetWord(0).IsOOV(), GetScoreProducerDescription()
                             << ": Missing Orientation property. "
                             << "Please check phrase table and glue rules.");
              // unknown word
              isUnkRule = true;
          }

          const WordsRange& prevWordsRange = prevHypo->GetCurrSourceRange();
          size_t prevStartPos = prevWordsRange.GetStartPos();
          size_t prevEndPos = prevWordsRange.GetEndPos();

          ++nonTerminalNumber;
      }
  }
      

  // add scores
//  newScores[0] = orientationPhraseProperty.GetLeftToRightProbabilityMono();
//  newScores[1] = orientationPhraseProperty.GetLeftToRightProbabilitySwap();
//  newScores[2] = orientationPhraseProperty.GetLeftToRightProbabilityDright();
//  newScores[3] = orientationPhraseProperty.GetLeftToRightProbabilityDleft();
//  newScores[4] = orientationPhraseProperty.GetRightToLeftProbabilityMono();
//  newScores[5] = orientationPhraseProperty.GetRightToLeftProbabilitySwap();
//  newScores[6] = orientationPhraseProperty.GetRightToLeftProbabilityDright();
//  newScores[7] = orientationPhraseProperty.GetRightToLeftProbabilityDleft();

  accumulator->PlusEquals(this, newScores);
}
 
}

