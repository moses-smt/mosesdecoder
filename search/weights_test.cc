#include "search/weights.hh"

#define BOOST_TEST_MODULE WeightTest
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

namespace search {
namespace {

#define CHECK_WEIGHT(value, string) \
  i = parsed.find(string); \
  BOOST_REQUIRE(i != parsed.end()); \
  BOOST_CHECK_CLOSE((value), i->second, 0.001);

BOOST_AUTO_TEST_CASE(parse) {
  // These are not real feature weights.  
  Weights w("rarity=0 phrase-SGT=0 phrase-TGS=9.45117 lhsGrhs=0 lexical-SGT=2.33833 lexical-TGS=-28.3317 abstract?=0 LanguageModel=3 lexical?=1 glue?=5");
  const boost::unordered_map<std::string, search::Score> &parsed = w.GetMap();
  boost::unordered_map<std::string, search::Score>::const_iterator i;
  CHECK_WEIGHT(0.0, "rarity");
  CHECK_WEIGHT(0.0, "phrase-SGT");
  CHECK_WEIGHT(9.45117, "phrase-TGS");
  CHECK_WEIGHT(2.33833, "lexical-SGT");
  BOOST_CHECK(parsed.end() == parsed.find("lm"));
  BOOST_CHECK_CLOSE(3.0, w.LM(), 0.001);
  CHECK_WEIGHT(-28.3317, "lexical-TGS");
  CHECK_WEIGHT(5.0, "glue?");
}

BOOST_AUTO_TEST_CASE(dot) {
  Weights w("rarity=0 phrase-SGT=0 phrase-TGS=9.45117 lhsGrhs=0 lexical-SGT=2.33833 lexical-TGS=-28.3317 abstract?=0 LanguageModel=3 lexical?=1 glue?=5");
  BOOST_CHECK_CLOSE(9.45117 * 3.0, w.DotNoLM("phrase-TGS=3.0"), 0.001);
  BOOST_CHECK_CLOSE(9.45117 * 3.0, w.DotNoLM("phrase-TGS=3.0 LanguageModel=10"), 0.001);
  BOOST_CHECK_CLOSE(9.45117 * 3.0 + 28.3317 * 17.4, w.DotNoLM("rarity=5 phrase-TGS=3.0 LanguageModel=10 lexical-TGS=-17.4"), 0.001);
}

} // namespace
} // namespace search
