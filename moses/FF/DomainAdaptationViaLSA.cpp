#include <vector>
#include "DomainAdaptationViaLSA.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"
#include "LSA.h"

using namespace std;

namespace Moses
{


DA_via_LSA::
DA_via_LSA(const std::string &line)
  : StatelessFeatureFunction(2, line)
{
  m_numScoreComponents = 1;
  m_numTuneableComponents = 1;
  ReadParameters();
}

void 
DA_via_LSA::
Load(AllOptions::ptr const& opts) 
{
  m_options = opts;
  m_model.open(m_bname, m_L1, m_L2);
}

void 
DA_via_LSA::
InitializeForInput(ttasksptr const& ttask)
{
  SPTR<ContextScope> const& scope = ttask->GetScope();
  SPTR<ScopeSpecific> local = scope->get<ScopeSpecific>(this,true);
  SPTR<std::vector<std::string> > context = ttask->GetContextWindow();
  if (context) local->init(&m_model, *context);
  t_scope_specific.reset(new SPTR<ScopeSpecific>);
  *t_scope_specific = local;

}
  
void 
DA_via_LSA::
EvaluateInIsolation(const Phrase &source,
		    const TargetPhrase &targetPhrase,
		    ScoreComponentCollection &scoreBreakdown,
		    ScoreComponentCollection &estimatedScores) const
{
  // std::cerr << "Hello World! " << HERE << endl;
}

void DA_via_LSA::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
    m_bname = value;
  } else if (key == "L1") {
    m_L1 = value;
  } else if (key == "L2") {
    m_L2 = value;
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

}

