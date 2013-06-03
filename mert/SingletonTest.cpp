#include "Singleton.h"

#define BOOST_TEST_MODULE MertSingleton
#include <boost/test/unit_test.hpp>

using namespace MosesTuning;

namespace
{

static int g_count = 0;

class Instance
{
public:
  Instance() {
    ++g_count;
  }
  ~Instance() {}
};

} // namespace

BOOST_AUTO_TEST_CASE(singleton_basic)
{
  Instance* instance1 = Singleton<Instance>::GetInstance();
  Instance* instance2 = Singleton<Instance>::GetInstance();
  Instance* instance3 = Singleton<Instance>::GetInstance();
  BOOST_REQUIRE(instance1 == instance2);
  BOOST_REQUIRE(instance2 == instance3);
  BOOST_CHECK_EQUAL(1, g_count);

  Singleton<Instance>::Delete();
}
