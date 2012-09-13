#include "Data.h"
#include "Scorer.h"
#include "ScorerFactory.h"

#define BOOST_TEST_MODULE MertData
#include <boost/test/unit_test.hpp>

#include <boost/scoped_ptr.hpp>

using namespace MosesTuning;

//very basic test of sharding
BOOST_AUTO_TEST_CASE(shard_basic) {
  boost::scoped_ptr<Scorer> scorer(ScorerFactory::getScorer("BLEU", ""));
  Data data(scorer.get());
  FeatureArray fa1, fa2, fa3, fa4;
  ScoreArray sa1, sa2, sa3, sa4;
  fa1.setIndex("1");
  fa2.setIndex("2");
  fa3.setIndex("3");
  fa4.setIndex("4");
  sa1.setIndex("1");
  sa2.setIndex("2");
  sa3.setIndex("3");
  sa4.setIndex("4");
  data.getFeatureData()->add(fa1);
  data.getFeatureData()->add(fa2);
  data.getFeatureData()->add(fa3);
  data.getFeatureData()->add(fa4);
  data.getScoreData()->add(sa1);
  data.getScoreData()->add(sa2);
  data.getScoreData()->add(sa3);
  data.getScoreData()->add(sa4);

  std::vector<Data> shards;
  data.createShards(2,0,"",shards);

  BOOST_CHECK_EQUAL(shards.size(),(std::size_t)2);
  BOOST_CHECK_EQUAL(shards[1].getFeatureData()->size(),(std::size_t)2);
}

BOOST_AUTO_TEST_CASE(init_feature_map_test) {
  boost::scoped_ptr<Scorer> scorer(ScorerFactory::getScorer("BLEU", ""));
  Data data(scorer.get());

  std::string s = " d: 0 -7.66174 0 0 -3.51621 0 0 lm: -41.3435 -40.3647 tm: -67.6349 -100.438 -27.6817 -23.4685 8.99907 w: -9 ";
  std::string expected = "d_0 d_1 d_2 d_3 d_4 d_5 d_6 lm_0 lm_1 tm_0 tm_1 tm_2 tm_3 tm_4 w_0 ";
  data.InitFeatureMap(s);
  BOOST_CHECK_EQUAL(expected, data.Features());
}
