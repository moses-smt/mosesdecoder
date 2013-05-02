#include <sstream>

#include "FFState.h"
#include "LexicalReordering.h"
#include "LexicalReorderingState.h"
#include "StaticData.h"

using namespace std;

namespace Moses
{
LexicalReordering::LexicalReordering(const std::string &line)
: StatefulFeatureFunction("LexicalReordering", line)
{
  std::cerr << "Initializing LexicalReordering.." << std::endl;

  vector<FactorType> f_factors, e_factors;
  string filePath;

  for (size_t i = 0; i < m_args.size(); ++i) {
    const vector<string> &args = m_args[i];

    if (args[0] == "type") {
      m_configuration = new LexicalReorderingConfiguration(args[1]);
      m_configuration->SetScoreProducer(this);
      m_modelTypeString = m_configuration->GetModelString();
    }
    else if (args[0] == "input-factor") {
      f_factors =Tokenize<FactorType>(args[1]);
    }
    else if (args[0] == "output-factor") {
      e_factors =Tokenize<FactorType>(args[1]);
    }
    else if (args[0] == "path") {
      filePath = args[1];
    }
    else {
      throw "Unknown argument " + args[0];
    }
  }

  switch(m_configuration->GetCondition()) {
    case LexicalReorderingConfiguration::FE:
    case LexicalReorderingConfiguration::E:
      m_factorsE = e_factors;
      if(m_factorsE.empty()) {
        UserMessage::Add("TL factor mask for lexical reordering is unexpectedly empty");
        exit(1);
      }
      if(m_configuration->GetCondition() == LexicalReorderingConfiguration::E)
        break; // else fall through
    case LexicalReorderingConfiguration::F:
      m_factorsF = f_factors;
      if(m_factorsF.empty()) {
        UserMessage::Add("SL factor mask for lexical reordering is unexpectedly empty");
        exit(1);
      }
      break;
    default:
      UserMessage::Add("Unknown conditioning option!");
      exit(1);
  }

  m_table = LexicalReorderingTable::LoadAvailable(filePath, m_factorsF, m_factorsE, std::vector<FactorType>());

}

LexicalReordering::~LexicalReordering()
{
  if(m_table)
    delete m_table;
  delete m_configuration;
}

Scores LexicalReordering::GetProb(const Phrase& f, const Phrase& e) const
{
  return m_table->GetScore(f, e, Phrase(ARRAY_SIZE_INCR));
}

FFState* LexicalReordering::Evaluate(const Hypothesis& hypo,
                                     const FFState* prev_state,
                                     ScoreComponentCollection* out) const
{
  Scores score(GetNumScoreComponents(), 0);
  const LexicalReorderingState *prev = dynamic_cast<const LexicalReorderingState *>(prev_state);
  LexicalReorderingState *next_state = prev->Expand(hypo.GetTranslationOption(), score);

  out->PlusEquals(this, score);

  return next_state;
}

const FFState* LexicalReordering::EmptyHypothesisState(const InputType &input) const
{
  return m_configuration->CreateLexicalReorderingState(input);
}

void LexicalReordering::Evaluate(const TargetPhrase &targetPhrase
                      , ScoreComponentCollection &scoreBreakdown
                      , float &estimatedFutureScore) const
{
  CHECK(false);
}

}

