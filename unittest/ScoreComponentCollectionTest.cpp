/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2010 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include <stdexcept>

#include <boost/test/unit_test.hpp>

#include "DummyScoreProducers.h"
#include "FeatureFunction.h"
#include "ScoreComponentCollection.h"

using namespace Moses;
using namespace std;

BOOST_AUTO_TEST_SUITE(scc)

class MockSingleFeature : public StatelessFeatureFunction {
  public:
    MockSingleFeature(): StatelessFeatureFunction("MockSingle") {}
    std::string GetScoreProducerWeightShortName() const {return "sf";}
    size_t GetNumScoreComponents() const {return 1;}
};

class MockMultiFeature : public StatelessFeatureFunction {
  public:
    MockMultiFeature(): StatelessFeatureFunction("MockMulti") {}
    std::string GetScoreProducerWeightShortName() const {return "mf";}
    size_t GetNumScoreComponents() const {return 5;}
};

class MockSparseFeature : public StatelessFeatureFunction {
  public:
    MockSparseFeature(): StatelessFeatureFunction("MockSparse") {}
    std::string GetScoreProducerWeightShortName() const {return "sf";}
    size_t GetNumScoreComponents() const {return ScoreProducer::unlimited;}
};


struct MockProducers {
  MockProducers() {}

  MockSingleFeature single;
  MockMultiFeature multi;
  MockSparseFeature sparse;
};

BOOST_FIXTURE_TEST_CASE(ctor, MockProducers) 
{
  ScoreComponentCollection scc;
  BOOST_CHECK_EQUAL(scc.GetScoreForProducer(&single),0);
  float expected[] =  {0,0,0,0,0};
  std::vector<float> actual= scc.GetScoresForProducer(&multi);
  BOOST_CHECK_EQUAL_COLLECTIONS(expected, expected+5, actual.begin(), actual.begin()+5);
}

BOOST_FIXTURE_TEST_CASE(plusequals, MockProducers)
{
  float arr1[] = {1,2,3,4,5};
  float arr2[] = {2,4,6,8,10};
  std::vector<float> vec1(arr1,arr1+5);
  std::vector<float> vec2(arr2,arr2+5);

  ScoreComponentCollection scc;
  scc.PlusEquals(&single, 3.4f);
  BOOST_CHECK_EQUAL(scc.GetScoreForProducer(&single), 3.4f);
  scc.PlusEquals(&multi,vec1);
  std::vector<float> actual = scc.GetScoresForProducer(&multi);
  BOOST_CHECK_EQUAL_COLLECTIONS(vec1.begin(),vec1.end()
        ,actual.begin(), actual.end());
  scc.PlusEquals(&multi,vec1);
  actual = scc.GetScoresForProducer(&multi);
  BOOST_CHECK_EQUAL_COLLECTIONS(vec2.begin(),vec2.end(),
         actual.begin(), actual.end());

  BOOST_CHECK_EQUAL(scc.GetScoreForProducer(&single), 3.4f);
}

BOOST_FIXTURE_TEST_CASE(sparse_feature, MockProducers)
{
  ScoreComponentCollection scc;
  scc.Assign(&sparse, "first", 1.3f);
  scc.Assign(&sparse, "second", 2.1f);
  BOOST_CHECK_EQUAL( scc.GetScoreForProducer(&sparse,"first"), 1.3f);
  BOOST_CHECK_EQUAL( scc.GetScoreForProducer(&sparse,"second"), 2.1f);
  BOOST_CHECK_EQUAL( scc.GetScoreForProducer(&sparse,"third"), 0.0f);
  scc.Assign(&sparse, "first", -1.9f);
  BOOST_CHECK_EQUAL( scc.GetScoreForProducer(&sparse,"first"), -1.9f);
  scc.PlusEquals(&sparse, "first", -1.9f);
  BOOST_CHECK_EQUAL( scc.GetScoreForProducer(&sparse,"first"), -3.8f);
}


BOOST_AUTO_TEST_SUITE_END()

