#include "FeatureData.h"

#define BOOST_TEST_MODULE FeatureData
#include <boost/test/unit_test.hpp>

#include <sstream>

using namespace MosesTuning;

namespace
{

void CheckFeatureMap(const FeatureData* feature_data,
                     const char* str, int num_feature, int* cnt)
{
  for (int i = 0; i < num_feature; ++i) {
    std::stringstream ss;
    ss << str << "_" << i;
    const std::string& s = ss.str();
    BOOST_CHECK_EQUAL(feature_data->getFeatureIndex(s), (std::size_t)(*cnt));
    BOOST_CHECK_EQUAL(feature_data->getFeatureName(*cnt).c_str(), s);
    ++(*cnt);
  }
}

} // namespace

BOOST_AUTO_TEST_CASE(set_feature_map)
{
  std::string str("d_0 d_1 d_2 d_3 d_4 d_5 d_6 lm_0 lm_1 tm_0 tm_1 tm_2 tm_3 tm_4 w_0 ");
  FeatureData feature_data;

  feature_data.setFeatureMap(str);

  BOOST_REQUIRE(feature_data.Features() == str);
  BOOST_REQUIRE(feature_data.NumberOfFeatures() == 15);

  int cnt = 0;
  CheckFeatureMap(&feature_data, "d", 7, &cnt);
  CheckFeatureMap(&feature_data, "lm", 2, &cnt);
  CheckFeatureMap(&feature_data, "tm", 5, &cnt);

  BOOST_CHECK_EQUAL(feature_data.getFeatureIndex("w_0"), (std::size_t)cnt);
  BOOST_CHECK_EQUAL(feature_data.getFeatureName(cnt).c_str(), "w_0");
}
