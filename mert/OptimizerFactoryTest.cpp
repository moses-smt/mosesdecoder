#include "OptimizerFactory.h"
#include "Optimizer.h"

#define BOOST_TEST_MODULE MertOptimizerFactory
#include <boost/test/unit_test.hpp>
#include <boost/scoped_ptr.hpp>

using namespace MosesTuning;

namespace
{

inline bool CheckBuildOptimizer(unsigned dim,
                                const std::vector<unsigned>& to_optimize,
                                const std::vector<bool>& positive,
                                const std::vector<parameter_t>& start,
                                const std::string& type,
                                unsigned int num_random)
{
  boost::scoped_ptr<Optimizer> optimizer(OptimizerFactory::BuildOptimizer(dim, to_optimize, positive, start, type, num_random));
  return optimizer.get() != NULL;
}

} // namespace

BOOST_AUTO_TEST_CASE(optimizer_type)
{
  BOOST_CHECK_EQUAL(OptimizerFactory::GetOptimizerType("powell"),
                    OptimizerFactory::POWELL);
  BOOST_CHECK_EQUAL(OptimizerFactory::GetOptimizerType("random"),
                    OptimizerFactory::RANDOM);
  BOOST_CHECK_EQUAL(OptimizerFactory::GetOptimizerType("random-direction"),
                    OptimizerFactory::RANDOM_DIRECTION);
}

BOOST_AUTO_TEST_CASE(optimizer_build)
{
  const unsigned dim = 3;
  std::vector<unsigned> to_optimize;
  to_optimize.push_back(1);
  to_optimize.push_back(2);
  to_optimize.push_back(3);
  std::vector<parameter_t> start;
  start.push_back(0.3);
  start.push_back(0.1);
  start.push_back(0.2);
  const unsigned int num_random = 1;
  std::vector<bool> positive(dim);
  for (unsigned int k = 0; k < dim; k++)
    positive[k] = false;

  BOOST_CHECK(CheckBuildOptimizer(dim, to_optimize, positive, start, "powell", num_random));
  BOOST_CHECK(CheckBuildOptimizer(dim, to_optimize, positive, start, "random", num_random));
  BOOST_CHECK(CheckBuildOptimizer(dim, to_optimize, positive, start, "random-direction", num_random));
}
