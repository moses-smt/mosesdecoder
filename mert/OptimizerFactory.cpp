#include "OptimizerFactory.h"
#include "Optimizer.h"

using namespace std;

namespace MosesTuning
{


vector<string> OptimizerFactory::m_type_names;

void OptimizerFactory::SetTypeNames()
{
  if (m_type_names.empty()) {
    m_type_names.resize(NOPTIMIZER);
    m_type_names[POWELL] = "powell";
    m_type_names[RANDOM_DIRECTION] = "random-direction";
    m_type_names[RANDOM] = "random";
    // Add new type there
  }
}
vector<string> OptimizerFactory::GetTypeNames()
{
  if (m_type_names.empty())
    SetTypeNames();
  return m_type_names;
}

OptimizerFactory::OptimizerType OptimizerFactory::GetOptimizerType(const string& type)
{
  unsigned int t;
  if (m_type_names.empty())
    SetTypeNames();
  for (t = 0; t < m_type_names.size(); t++)
    if (m_type_names[t] == type)
      break;
  return((OptimizerType)t);
}

Optimizer* OptimizerFactory::BuildOptimizer(unsigned dim,
    const vector<unsigned>& i2o,
    const std::vector<bool>& positive,
    const vector<parameter_t>& start,
    const string& type,
    unsigned int nrandom)
{
  OptimizerType opt_type = GetOptimizerType(type);
  if (opt_type == NOPTIMIZER) {
    cerr << "Error: unknown Optimizer type " << type << endl;
    cerr << "Known Algorithm are:" << endl;
    unsigned int t;
    for (t = 0; t < m_type_names.size(); t++)
      cerr << m_type_names[t] << endl;
    throw ("unknown Optimizer Type");
  }

  switch (opt_type) {
  case POWELL:
    return new SimpleOptimizer(dim, i2o, positive, start, nrandom);
    break;
  case RANDOM_DIRECTION:
    return new RandomDirectionOptimizer(dim, i2o, positive, start, nrandom);
    break;
  case RANDOM:
    return new RandomOptimizer(dim, i2o, positive, start, nrandom);
    break;
  default:
    cerr << "Error: unknown optimizer" << type << endl;
    return NULL;
  }
}

}
