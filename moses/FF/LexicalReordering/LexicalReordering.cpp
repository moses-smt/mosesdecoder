#include <sstream>

#include "moses/FF/FFState.h"
#include "LexicalReordering.h"
#include "LexicalReorderingState.h"
#include "moses/StaticData.h"

using namespace std;

namespace Moses
{
LexicalReordering::LexicalReordering(const std::string &line)
  : StatefulFeatureFunction(line)
{
  std::cerr << "Initializing LexicalReordering.." << std::endl;

  for (size_t i = 0; i < m_args.size(); ++i) {
    const vector<string> &args = m_args[i];

    if (args[0] == "type") {
      m_configuration = new LexicalReorderingConfiguration(args[1]);
      m_configuration->SetScoreProducer(this);
      m_modelTypeString = m_configuration->GetModelString();
    } else if (args[0] == "input-factor") {
      m_factorsF =Tokenize<FactorType>(args[1]);
    } else if (args[0] == "output-factor") {
      m_factorsE =Tokenize<FactorType>(args[1]);
    } else if (args[0] == "path") {
      m_filePath = args[1];
    } else {
      throw "Unknown argument " + args[0];
    }
  }

  switch(m_configuration->GetCondition()) {
  case LexicalReorderingConfiguration::FE:
  case LexicalReorderingConfiguration::E:
    if(m_factorsE.empty()) {
      throw "TL factor mask for lexical reordering is unexpectedly empty";
    }
    if(m_configuration->GetCondition() == LexicalReorderingConfiguration::E)
      break; // else fall through
  case LexicalReorderingConfiguration::F:
    if(m_factorsF.empty()) {
      throw "SL factor mask for lexical reordering is unexpectedly empty";
    }
    break;
  default:
    throw "Unknown conditioning option!";
  }
}

LexicalReordering::~LexicalReordering()
{
  if(m_table)
    delete m_table;
  delete m_configuration;
}

void LexicalReordering::Load()
{
  m_table = LexicalReorderingTable::LoadAvailable(m_filePath, m_factorsF, m_factorsE, std::vector<FactorType>());
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

bool LexicalReordering::IsUseable(const FactorMask &mask) const
{
  for (size_t i = 0; i < m_factorsE.size(); ++i) {
    const FactorType &factor = m_factorsE[i];
    if (!mask[factor]) {
      return false;
    }
  }
  return true;

}
}

