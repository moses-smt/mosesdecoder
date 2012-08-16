#include <sstream>

#include "FFState.h"
#include "LexicalReordering.h"
#include "LexicalReorderingState.h"
#include "StaticData.h"

namespace Moses
{

LexicalReordering::LexicalReordering(std::vector<FactorType>& f_factors,
                                     std::vector<FactorType>& e_factors,
                                     const std::string &modelType,
                                     const std::string &filePath,
                                     const std::vector<float>& weights)
  : m_configuration(this, modelType), m_refCount(new size_t(1)) // MJD: Reference counting
{
  std::cerr << "Creating lexical reordering...\n";
  std::cerr << "weights: ";
  for(size_t w = 0; w < weights.size(); ++w) {
    std::cerr << weights[w] << " ";
  }
  std::cerr << "\n";

  m_modelTypeString = modelType;

  switch(m_configuration.GetCondition()) {
  case LexicalReorderingConfiguration::FE:
  case LexicalReorderingConfiguration::E:
    m_factorsE = e_factors;
    if(m_factorsE.empty()) {
      UserMessage::Add("TL factor mask for lexical reordering is unexpectedly empty");
      exit(1);
    }
    if(m_configuration.GetCondition() == LexicalReorderingConfiguration::E)
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


  if(weights.size() != m_configuration.GetNumScoreComponents()) {
    std::ostringstream os;
    os << "Lexical reordering model (type " << modelType << "): expected " << m_numScoreComponents << " weights, got " << weights.size() << std::endl;
    UserMessage::Add(os.str());
    exit(1);
  }

  // add ScoreProducer - don't do this before our object is set up
  const_cast<ScoreIndexManager&>(StaticData::Instance().GetScoreIndexManager()).AddScoreProducer(this);
  const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);

  m_table = LexicalReorderingTable::LoadAvailable(filePath, m_factorsF, m_factorsE, std::vector<FactorType>());
}

// MJD: Added copy constructor with reference counting
LexicalReordering::LexicalReordering(const LexicalReordering& l) :
    StatefulFeatureFunction(l),
    m_configuration(this, l.m_modelTypeString),
    m_table(l.m_table),
    m_modelTypeString(l.m_modelTypeString),
    m_modelType(l.m_modelType),
    m_numScoreComponents(l.m_numScoreComponents),
    m_condition(l.m_condition),
    m_factorsE(l.m_factorsE), m_factorsF(l.m_factorsF),
    m_refCount(l.m_refCount)
{
  (*m_refCount)++; 
}

// MJD: Added reference counting
LexicalReordering::~LexicalReordering()
{
  (*m_refCount)--;
  if(*m_refCount == 0) {
    if(m_table)
      delete m_table;
    delete m_refCount;
  }
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
  return m_configuration.CreateLexicalReorderingState(input);
}

}

