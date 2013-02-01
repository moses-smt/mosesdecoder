#include <sstream>

#include "FFState.h"
#include "LexicalReordering.h"
#include "LexicalReorderingState.h"
#include "StaticData.h"

using namespace std;

namespace Moses
{
LexicalReordering::LexicalReordering(const std::string &line)
: StatefulFeatureFunction("LexicalReordering", 6, line)
{
  std::cerr << "Initializing LexicalReordering.." << std::endl;

  vector<string> tokens = Tokenize(line);

  m_configuration = new LexicalReorderingConfiguration(tokens[1]);
  m_configuration->SetScoreProducer(this);
  m_modelTypeString = m_configuration->GetModelString();

  vector<FactorType> f_factors = Tokenize<FactorType>(tokens[2]);
  vector<FactorType> e_factors = Tokenize<FactorType>(tokens[3]);

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

  const string &filePath = tokens[4];

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

}

