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

#include "moses/FF/StatelessFeatureFunction.h"
#include "ScoreComponentCollection.h"

using namespace Moses;
using namespace std;

BOOST_AUTO_TEST_SUITE(scc)

class MockStatelessFeatureFunction : public StatelessFeatureFunction
{
public:
  MockStatelessFeatureFunction(size_t n, const string &line) :
    StatelessFeatureFunction(n, line) {}
  void Evaluate(const Hypothesis&, ScoreComponentCollection*) const {}
  void EvaluateChart(const ChartHypothesis&, ScoreComponentCollection*) const {}
  void Evaluate(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore) const
  {}
  void Evaluate(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const
  {}

};

class MockSingleFeature : public MockStatelessFeatureFunction
{
public:
  MockSingleFeature(): MockStatelessFeatureFunction(1, "MockSingle") {}

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }
};

class MockMultiFeature : public MockStatelessFeatureFunction
{
public:
  MockMultiFeature(): MockStatelessFeatureFunction(5, "MockMulti") {}

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

};

class MockSparseFeature : public MockStatelessFeatureFunction
{
public:
  MockSparseFeature(): MockStatelessFeatureFunction(0, "MockSparse") {}

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }
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
  scc.PlusEquals(&sparse, StringPiece("first"), -1.9f);
  BOOST_CHECK_EQUAL( scc.GetScoreForProducer(&sparse,"first"), -3.8f);
}

/*
 Doesn't work because of the static registration of ScoreProducers
 in ScoreComponentCollection.
BOOST_FIXTURE_TEST_CASE(save, MockProducers)
{
  ScoreComponentCollection scc;
  scc.Assign(&sparse, "first", 1.1f);
  scc.Assign(&single, 0.25f);
  float arr[] = {1,2.1,3,4,5};
  std::vector<float> vec1(arr,arr+5);
  scc.Assign(&multi,vec1);
  ostringstream out;
  scc.Save(out);
  cerr << out.str() << endl;
  istringstream in (out.str());
  string line;
  getline(in,line);
  BOOST_CHECK_EQUAL(line, "MockSingle:4_1 0.25");
  getline(in,line);
  BOOST_CHECK_EQUAL(line, "MockMulti:4_1 1");
  getline(in,line);
  BOOST_CHECK_EQUAL(line, "MockMulti:4_2 2.1");
  getline(in,line);
  BOOST_CHECK_EQUAL(line, "MockMulti:4_3 3");
  getline(in,line);
  BOOST_CHECK_EQUAL(line, "MockMulti:4_4 4");
  getline(in,line);
  BOOST_CHECK_EQUAL(line, "MockMulti:4_5 5");
  getline(in,line);
  BOOST_CHECK_EQUAL(line,"MockSparse:4_first 1.1");
  BOOST_CHECK(!getline(in,line));
}
*/


BOOST_AUTO_TEST_SUITE_END()

