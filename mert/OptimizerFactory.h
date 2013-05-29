#ifndef MERT_OPTIMIZER_FACTORY_H_
#define MERT_OPTIMIZER_FACTORY_H_

#include <vector>
#include "Types.h"

namespace MosesTuning
{


class Optimizer;

class OptimizerFactory
{
public:
  // NOTE: Add new optimizer here BEFORE NOPTIMZER
  enum OptimizerType {
    POWELL = 0,
    RANDOM_DIRECTION = 1,
    RANDOM,
    NOPTIMIZER
  };

  static std::vector<std::string> GetTypeNames();

  // Setup optimization types.
  static void SetTypeNames();

  // Get optimizer type.
  static OptimizerType GetOptimizerType(const std::string& type);

  static Optimizer* BuildOptimizer(unsigned dim,
                                   const std::vector<unsigned>& to_optimize,
                                   const std::vector<bool>& positive,
                                   const std::vector<parameter_t>& start,
                                   const std::string& type,
                                   unsigned int nrandom);

private:
  OptimizerFactory() {}
  ~OptimizerFactory() {}

  static std::vector<std::string> m_type_names;
};

}


#endif  // MERT_OPTIMIZER_FACTORY_H_
