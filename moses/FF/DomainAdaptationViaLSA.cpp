#include <vector>
#include "DomainAdaptationViaLSA.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"

using namespace std;

namespace Moses
{


DA_via_LSA::
DA_via_LSA(const std::string &line)
  : StatelessFeatureFunction(2, line)
{
  ReadParameters();
}

void 
Load(AllOptions::ptr const& opts) 
{
  m_options = opts;
  m_term_vectors.open(m_term_vector_file);
  char const* p = m_term_vectors.data();
  m_num_term_vectors = *reinterpret_cast<uint64_t const*>(p);
  p += 8;
  m_num_lsa_dimensions = *reinterpret_cast<uint32_t const*>(p);
  m_tvec = p;
  m_V.open(m_vocab_file);
}

void 
DA_via_LSA::
InitializeForInput(ttasksptr const& ttask)
{
  SPTR<ContextScope> const& scope = ttask->GetScope();
  SPTR<ScopeSpecific>& local = t_scope_specific;
  local = scope->get<ScopeSpecific>(this, true);
  if (local->weights.size() == 0) 
    
    local->weights.resize(m_num_term_vectors,1);
   = ttaks.
}
  
void 
DA_via_LSA::
EvaluateInIsolation(const Phrase &source,
		    const TargetPhrase &targetPhrase
		    ScoreComponentCollection &scoreBreakdown,
		    ScoreComponentCollection &estimatedScores) const
{
  // dense scores
  vector<float> newScores(m_numScoreComponents);
  newScores[0] = 1.5;
  newScores[1] = 0.3;
  scoreBreakdown.PlusEquals(this, newScores);

  // sparse scores
  scoreBreakdown.PlusEquals(this, "sparse-name", 2.4);

}

#if 0
void 
DA_via_LSA::
EvaluateWithSourceContext(const InputType &input,
			  const InputPath &inputPath,
			  const TargetPhrase &targetPhrase,
			  const StackVec *stackVec,
			  ScoreComponentCollection &scoreBreakdown,
			  ScoreComponentCollection *estimatedScores) const
{
  if (targetPhrase.GetNumNonTerminals()) {
    vector<float> newScores(m_numScoreComponents);
    newScores[0] = - std::numeric_limits<float>::infinity();
    scoreBreakdown.PlusEquals(this, newScores);
  }
}
#endif

void DA_via_LSA::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "vocab") {
    m_vocab_file = value;
  } else if (key == "term-vectors") {
    m_term_vector_file = value;
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

}

