#if 1
#include <vector>
#include "DomainAdaptationViaLSA.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>


using namespace std;

namespace Moses
{


DA_via_LSA::
DA_via_LSA(const std::string &line)
  : StatelessFeatureFunction(2, line)
{
  m_numScoreComponents = 1;
  m_numTuneableComponents = 1;
  m_total_docs = 0;
  ReadParameters();
}

void 
DA_via_LSA::
Load(AllOptions::ptr const& opts) 
{
  m_options = opts;

  using boost::property_tree::ptree;
  ptree pt;
  read_json(m_path, pt);
  m_bname = pt.get<std::string>("path");
  m_L1 = pt.get<std::string>("L1");
  m_L2 = pt.get<std::string>("L2");
  m_total_docs = pt.get<size_t>("total-docs");

  // fprintf(stderr,"%s%s %s%s %z\n",
  // 	 m_bname.c_str(), m_L1.c_str(),
  // 	 m_bname.c_str(), m_L2.c_str(),
  // 	 m_total_docs);
  m_model.open(m_bname, m_L1, m_L2, m_total_docs);
}

void 
DA_via_LSA::
InitializeForInput(ttasksptr const& ttask)
{
  SPTR<ContextScope> const& scope = ttask->GetScope();
  SPTR<ScopeSpecific> local = scope->get<ScopeSpecific>(this,true);
  SPTR<std::vector<std::string> > context = ttask->GetContextWindow();
  if (context)
    {
      local->init(&m_model, *context);
    }
  else if (ttask->GetSource()->GetType() == SentenceInput)
    {
      local->init(&m_model, *ttask->GetSource());
    }
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
    m_path = value;
  } else if (key == "L1") {
    m_L1 = value;
  } else if (key == "L2") {
    m_L2 = value;
  } else if (key == "total_docs") {
    m_total_docs = atoi(value.c_str());
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

}

#endif
