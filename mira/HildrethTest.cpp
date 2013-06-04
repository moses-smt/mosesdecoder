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

#include <cstdio>
#include <cstdlib>
#include <string>

#include <boost/test/unit_test.hpp>

#include "Hildreth.h"
#include "Optimiser.h"
#include "ScoreComponentCollection.h"

using namespace std;
using namespace Moses;
using namespace Mira;

namespace MosesTest
{

class MockSingleFeature : public StatelessFeatureFunction
{
public:
  MockSingleFeature(): StatelessFeatureFunction("MockSingle",1) {}
  std::string GetScoreProducerWeightShortName(unsigned) const {
    return "sf";
  }
};

class MockMultiFeature : public StatelessFeatureFunction
{
public:
  MockMultiFeature(): StatelessFeatureFunction("MockMulti",5) {}
  std::string GetScoreProducerWeightShortName(unsigned) const {
    return "mf";
  }
};

class MockSparseFeature : public StatelessFeatureFunction
{
public:
  MockSparseFeature(): StatelessFeatureFunction("MockSparse", ScoreProducer::unlimited) {}
  std::string GetScoreProducerWeightShortName(unsigned) const {
    return "sf";
  }
};

struct MockProducers {
  MockProducers() {}

  MockSingleFeature single;
  MockMultiFeature multi;
  MockSparseFeature sparse;
};



BOOST_AUTO_TEST_SUITE(hildreth_test)

BOOST_FIXTURE_TEST_CASE(test_hildreth_1, MockProducers)
{
  // Feasible example with 2 constraints
  cerr << "\n>>>>>Hildreth test, without slack and with 0.01 slack" << endl << endl;
  vector< ScoreComponentCollection> featureValueDiffs;
  vector< float> lossMinusModelScoreDiff;

  // initial weights
  float w[] = { 1, 1, 1, 1, 0 };
  vector<float> vec(w,w+5);
  ScoreComponentCollection weights;
  weights.PlusEquals(&multi, vec);

  // feature values (second is oracle)
  //float arr1[] = {0, -5, -27.0908, -1.83258, 0 };
  //float arr2[] = {0, -5, -29.158, -1.83258, 0 };
  //float arr3[] = {0, -5, -27.0908, -1.83258, 0 };

  // feature value differences (to oracle)
  ScoreComponentCollection s1, s2, s3;
  float arr1[] = { 0, 0, -2.0672, 0, 0 };
  float arr2[] = { 0, 0, 0, 0, 0 };
  float arr3[] = { 0, 0, -2.0672, 0, 0 };

  float loss1 = 2.34085;
  float loss2 = 0;
  float loss3 = 2.34085;

  vector<float> vec1(arr1,arr1+5);
  vector<float> vec2(arr2,arr2+5);
  vector<float> vec3(arr3,arr3+5);

  s1.PlusEquals(&multi,vec1);
  s2.PlusEquals(&multi,vec2);
  s3.PlusEquals(&multi,vec3);

  featureValueDiffs.push_back(s1);
  featureValueDiffs.push_back(s2);
  featureValueDiffs.push_back(s3);

  cerr << "feature value diff: " << featureValueDiffs[0] << endl;
  cerr << "feature value diff: " << featureValueDiffs[1] << endl;
  cerr << "feature value diff: " << featureValueDiffs[2] << endl << endl;

  float oldModelScoreDiff1 = featureValueDiffs[0].InnerProduct(weights);
  float oldModelScoreDiff2 = featureValueDiffs[1].InnerProduct(weights);
  float oldModelScoreDiff3 = featureValueDiffs[2].InnerProduct(weights);

  cerr << "model score diff: " << oldModelScoreDiff1 << ", loss: " << loss1 << endl;
  cerr << "model score diff: " << oldModelScoreDiff2 << ", loss: " << loss2 << endl;
  cerr << "model score diff: " << oldModelScoreDiff3 << ", loss: " << loss3 << endl << endl;

  lossMinusModelScoreDiff.push_back(loss1 - oldModelScoreDiff1);
  lossMinusModelScoreDiff.push_back(loss2 - oldModelScoreDiff2);
  lossMinusModelScoreDiff.push_back(loss3 - oldModelScoreDiff3);

  vector< float> alphas1 = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiff);
  vector< float> alphas2 = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiff, 0.01);

  cerr << "\nalphas without slack:" << endl;
  for (size_t i = 0; i < alphas1.size(); ++i) {
    cerr << "alpha " << i << ": " << alphas1[i] << endl;
  }
  cerr << endl;

  cerr << "partial updates:" << endl;
  vector< ScoreComponentCollection> featureValueDiffs1(featureValueDiffs);
  FVector totalUpdate1 = ScoreComponentCollection::CreateFVector();
  for (size_t k = 0; k < featureValueDiffs1.size(); ++k) {
    featureValueDiffs1[k].MultiplyEquals(alphas1[k]);
    cerr << k << ": " << featureValueDiffs1[k].GetScoresVector() << endl;
    FVector update = featureValueDiffs1[k].GetScoresVector();
    totalUpdate1 += update;
  }
  cerr << endl;
  cerr << "total update: " << totalUpdate1 << endl << endl;

  ScoreComponentCollection weightsUpdate1(weights);
  weightsUpdate1.PlusEquals(totalUpdate1);
  cerr << "new weights: " << weightsUpdate1 << endl << endl;

  float newModelScoreDiff1 = featureValueDiffs[0].InnerProduct(weightsUpdate1);
  float newModelScoreDiff2 = featureValueDiffs[1].InnerProduct(weightsUpdate1);
  float newModelScoreDiff3 = featureValueDiffs[2].InnerProduct(weightsUpdate1);

  cerr << "new model score diff: " << newModelScoreDiff1 << ", loss: " << loss1 << endl;
  cerr << "new model score diff: " << newModelScoreDiff2 << ", loss: " << loss2 << endl;
  cerr << "new model score diff: " << newModelScoreDiff3 << ", loss: " << loss3 << endl;

  cerr << "\n\nalphas with slack 0.01:" << endl;
  for (size_t i = 0; i < alphas2.size(); ++i) {
    cerr << "alpha " << i << ": " << alphas2[i] << endl;
  }
  cerr << endl;

  cerr << "partial updates:" << endl;
  vector< ScoreComponentCollection> featureValueDiffs2(featureValueDiffs);
  FVector totalUpdate2 = ScoreComponentCollection::CreateFVector();
  for (size_t k = 0; k < featureValueDiffs2.size(); ++k) {
    featureValueDiffs2[k].MultiplyEquals(alphas2[k]);
    cerr << k << ": " << featureValueDiffs2[k].GetScoresVector() << endl;
    FVector update = featureValueDiffs2[k].GetScoresVector();
    totalUpdate2 += update;
  }
  cerr << endl;
  cerr << "total update: " << totalUpdate2 << endl << endl;

  ScoreComponentCollection weightsUpdate2(weights);
  weightsUpdate2.PlusEquals(totalUpdate2);
  cerr << "new weights: " << weightsUpdate2 << endl << endl;

  float newModelScoreDiff4 = featureValueDiffs[0].InnerProduct(weightsUpdate2);
  float newModelScoreDiff5 = featureValueDiffs[1].InnerProduct(weightsUpdate2);
  float newModelScoreDiff6 = featureValueDiffs[2].InnerProduct(weightsUpdate2);

  cerr << "new model score diff: " << newModelScoreDiff4 << ", loss: " << loss1 << endl;
  cerr << "new model score diff: " << newModelScoreDiff5 << ", loss: " << loss2 << endl;
  cerr << "new model score diff: " << newModelScoreDiff6 << ", loss: " << loss3 << endl;
}


BOOST_FIXTURE_TEST_CASE(test_hildreth_3, MockProducers)
{
  // Unfeasible example with 21 constraints
  cerr << "\n>>>>>Hildreth test, without slack and with 0.01 slack" << endl << endl;
  vector< ScoreComponentCollection> featureValueDiffs;
  vector< float> lossMinusModelScoreDiff;

  // initial weights
  float w[] = { 1, 1, 0.638672, 1, 0 };
  vector<float> vec(w,w+5);
  ScoreComponentCollection weights;
  weights.PlusEquals(&multi, vec);

  int numberOfConstraints = 21;

  // feature value differences (to oracle)
  // NOTE: these feature values are only approximations
  ScoreComponentCollection s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15, s16, s17, s18, s19, s20, s21;
  float arr1[] = { 0, 0, -2.0672, 0, 0 };
  float arr2[] = { 0, 0, 0, 0, 0 };
  float arr3[] = { 0, 0, -2.08436, 1.38629, 0 };
  float arr4[] = { 0, 0, -0.0171661, 1.38629, 0 };
  float arr5[] = { 0, 0, 4.4283, 0, 0 };
  float arr6[] = { 0, 0, 3.84829, 1.38629, 0 };
  float arr7[] = { 0, 0, 6.83689, 0, 0 };
  float arr8[] = { 0, 0, 0, 0, 0 };
  float arr9[] = { 0, 0, -2.0672, 0, 0 };
  float arr10[] = { 0, 0, -0.0171661, 1.38629, 0 };
  float arr11[] = { 0, 0, -2.08436, 1.38629, 0 };
  float arr12[] = { 0, 0, 4.4283, 0, 0 };
  float arr13[] = { 3, 0, 2.41089, 0, 0 };
  float arr14[] = { 3, 0, 2.32709, 0, 0 };
  float arr15[] = { 0, 0, -2.0672, 0, 0 };
  float arr16[] = { 0, 0, -2.08436, 1.38629, 0 };
  float arr17[] = { 0, 0, 4.4283, 0, 0 };
  float arr18[] = { 0, 0, 3.84829, 1.38629, 0 };
  float arr19[] = { 0, 0, -0.0171661, 1.38629, 0 };
  float arr20[] = { 0, 0, 0, 0, 0 };
  float arr21[] = { 0, 0, 6.83689, 0, 0 };

  vector<float> losses;
  losses.push_back(2.73485);
  losses.push_back(0);
  losses.push_back(3.64118);
  losses.push_back(1.47347);
  losses.push_back(3.64118);
  losses.push_back(4.16278);
  losses.push_back(3.13952);
  losses.push_back(0);
  losses.push_back(2.73485);
  losses.push_back(1.47347);
  losses.push_back(3.64118);
  losses.push_back(3.64118);
  losses.push_back(2.51662);
  losses.push_back(2.73485);
  losses.push_back(2.73485);
  losses.push_back(3.64118);
  losses.push_back(3.64118);
  losses.push_back(4.16278);
  losses.push_back(1.47347);
  losses.push_back(0);
  losses.push_back(3.13952);

  vector<float> vec1(arr1,arr1+5);
  vector<float> vec2(arr2,arr2+5);
  vector<float> vec3(arr3,arr3+5);
  vector<float> vec4(arr4,arr4+5);
  vector<float> vec5(arr5,arr5+5);
  vector<float> vec6(arr6,arr6+5);
  vector<float> vec7(arr7,arr7+5);
  vector<float> vec8(arr8,arr8+5);
  vector<float> vec9(arr9,arr9+5);
  vector<float> vec10(arr10,arr10+5);
  vector<float> vec11(arr11,arr11+5);
  vector<float> vec12(arr12,arr12+5);
  vector<float> vec13(arr13,arr13+5);
  vector<float> vec14(arr14,arr14+5);
  vector<float> vec15(arr15,arr15+5);
  vector<float> vec16(arr16,arr16+5);
  vector<float> vec17(arr17,arr17+5);
  vector<float> vec18(arr18,arr18+5);
  vector<float> vec19(arr19,arr19+5);
  vector<float> vec20(arr20,arr20+5);
  vector<float> vec21(arr21,arr21+5);

  s1.PlusEquals(&multi,vec1);
  s2.PlusEquals(&multi,vec2);
  s3.PlusEquals(&multi,vec3);
  s4.PlusEquals(&multi,vec4);
  s5.PlusEquals(&multi,vec5);
  s6.PlusEquals(&multi,vec6);
  s7.PlusEquals(&multi,vec7);
  s8.PlusEquals(&multi,vec8);
  s9.PlusEquals(&multi,vec9);
  s10.PlusEquals(&multi,vec10);
  s11.PlusEquals(&multi,vec11);
  s12.PlusEquals(&multi,vec12);
  s13.PlusEquals(&multi,vec13);
  s14.PlusEquals(&multi,vec14);
  s15.PlusEquals(&multi,vec15);
  s16.PlusEquals(&multi,vec16);
  s17.PlusEquals(&multi,vec17);
  s18.PlusEquals(&multi,vec18);
  s19.PlusEquals(&multi,vec19);
  s20.PlusEquals(&multi,vec20);
  s21.PlusEquals(&multi,vec21);

  featureValueDiffs.push_back(s1);
  featureValueDiffs.push_back(s2);
  featureValueDiffs.push_back(s3);
  featureValueDiffs.push_back(s4);
  featureValueDiffs.push_back(s5);
  featureValueDiffs.push_back(s6);
  featureValueDiffs.push_back(s7);
  featureValueDiffs.push_back(s8);
  featureValueDiffs.push_back(s9);
  featureValueDiffs.push_back(s10);
  featureValueDiffs.push_back(s11);
  featureValueDiffs.push_back(s12);
  featureValueDiffs.push_back(s13);
  featureValueDiffs.push_back(s14);
  featureValueDiffs.push_back(s15);
  featureValueDiffs.push_back(s16);
  featureValueDiffs.push_back(s17);
  featureValueDiffs.push_back(s18);
  featureValueDiffs.push_back(s19);
  featureValueDiffs.push_back(s20);
  featureValueDiffs.push_back(s21);

  vector<float> oldModelScoreDiff;
  for (int i = 0; i < numberOfConstraints; ++i) {
    oldModelScoreDiff.push_back(featureValueDiffs[i].InnerProduct(weights));
  }

  for (int i = 0; i < numberOfConstraints; ++i) {
    cerr << "old model score diff: " << oldModelScoreDiff[i] << ", loss: " << losses[i] << "\t" << (oldModelScoreDiff[i] >= losses[i] ? 1 : 0) << endl;
  }

  for (int i = 0; i < numberOfConstraints; ++i) {
    lossMinusModelScoreDiff.push_back(losses[i] - oldModelScoreDiff[i]);
  }

  for (int i = 0; i < numberOfConstraints; ++i) {
    cerr << "A: " << featureValueDiffs[i] << ", b: " << lossMinusModelScoreDiff[i] << endl;
  }

  vector< float> alphas1 = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiff);
  vector< float> alphas2 = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiff, 0.01);

  cerr << "\nalphas without slack:" << endl;
  for (size_t i = 0; i < alphas1.size(); ++i) {
    cerr << "alpha " << i << ": " << alphas1[i] << endl;
  }
  cerr << endl;

  cerr << "partial updates:" << endl;
  vector< ScoreComponentCollection> featureValueDiffs1(featureValueDiffs);
  FVector totalUpdate1 = ScoreComponentCollection::CreateFVector();
  for (size_t k = 0; k < featureValueDiffs1.size(); ++k) {
    featureValueDiffs1[k].MultiplyEquals(alphas1[k]);
    cerr << k << ": " << featureValueDiffs1[k].GetScoresVector() << endl;
    FVector update = featureValueDiffs1[k].GetScoresVector();
    totalUpdate1 += update;
  }
  cerr << endl;
  cerr << "total update: " << totalUpdate1 << endl << endl;

  ScoreComponentCollection weightsUpdate1(weights);
  weightsUpdate1.PlusEquals(totalUpdate1);
  cerr << "old weights: " << weights << endl;
  cerr << "new weights: " << weightsUpdate1 << endl << endl;

  vector<float> newModelScoreDiff;
  for (int i = 0; i < numberOfConstraints; ++i) {
    newModelScoreDiff.push_back(featureValueDiffs[i].InnerProduct(weightsUpdate1));
  }

  for (int i = 0; i < numberOfConstraints; ++i) {
    cerr << "new model score diff: " << newModelScoreDiff[i] << ", loss: " << losses[i] << "\t" << (newModelScoreDiff[i] >= losses[i] ? 1 : 0) << endl;
  }

  cerr << "\n\nalphas with slack 0.01:" << endl;
  for (size_t i = 0; i < alphas2.size(); ++i) {
    cerr << "alpha " << i << ": " << alphas2[i] << endl;
  }
  cerr << endl;

  cerr << "partial updates:" << endl;
  vector< ScoreComponentCollection> featureValueDiffs2(featureValueDiffs);
  FVector totalUpdate2 = ScoreComponentCollection::CreateFVector();
  for (size_t k = 0; k < featureValueDiffs2.size(); ++k) {
    featureValueDiffs2[k].MultiplyEquals(alphas2[k]);
    cerr << k << ": " << featureValueDiffs2[k].GetScoresVector() << endl;
    FVector update = featureValueDiffs2[k].GetScoresVector();
    totalUpdate2 += update;
  }
  cerr << endl;
  cerr << "total update: " << totalUpdate2 << endl << endl;

  ScoreComponentCollection weightsUpdate2(weights);
  weightsUpdate2.PlusEquals(totalUpdate2);
  cerr << "old weights: " << weights << endl;
  cerr << "new weights: " << weightsUpdate2 << endl << endl;

  newModelScoreDiff.clear();
  for (int i = 0; i < numberOfConstraints; ++i) {
    newModelScoreDiff.push_back(featureValueDiffs[i].InnerProduct(weightsUpdate2));
  }

  for (int i = 0; i < numberOfConstraints; ++i) {
    cerr << "new model score diff: " << newModelScoreDiff[i] << ", loss: " << losses[i] << endl;
  }
}

BOOST_FIXTURE_TEST_CASE(test_hildreth_4, MockProducers)
{
  // Feasible example with 8 constraints
  cerr << "\n>>>>>Hildreth test, without slack and with 0.01 slack" << endl << endl;
  vector< ScoreComponentCollection> featureValueDiffs;
  vector< float> lossMinusModelScoreDiff;

  // initial weights
  float w[] = { 1, 1, 0.638672, 1, 0 };
  vector<float> vec(w,w+5);
  ScoreComponentCollection weights;
  weights.PlusEquals(&multi, vec);

  int numberOfConstraints = 8;

  // feature value differences (to oracle)
  // NOTE: these feature values are only approximations
  ScoreComponentCollection s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15, s16, s17, s18, s19, s20, s21;
  float arr1[] = { 0, 0, -2.0672, 0, 0 };
  float arr2[] = { 0, 0, 0, 0, 0 };
  float arr3[] = { 0, 0, -2.08436, 1.38629, 0 };
  float arr4[] = { 0, 0, -0.0171661, 1.38629, 0 };
//	float arr5[] = { 0, 0, 4.4283, 0, 0 };
//	float arr6[] = { 0, 0, 3.84829, 1.38629, 0 };
//	float arr7[] = { 0, 0, 6.83689, 0, 0 };

  float arr8[] = { 0, 0, 0, 0, 0 };
  float arr9[] = { 0, 0, -2.0672, 0, 0 };
//	float arr10[] = { 0, 0, -0.0171661, 1.38629, 0 };
//	float arr11[] = { 0, 0, -2.08436, 1.38629, 0 };
//	float arr12[] = { 0, 0, 4.4283, 0, 0 };
//	float arr13[] = { 3, 0, 2.41089, 0, 0 };
//	float arr14[] = { 3, 0, 2.32709, 0, 0 };

  float arr15[] = { 0, 0, -2.0672, 0, 0 };
  float arr16[] = { 0, 0, -2.08436, 1.38629, 0 };
//	float arr17[] = { 0, 0, 4.4283, 0, 0 };
//	float arr18[] = { 0, 0, 3.84829, 1.38629, 0 };
//	float arr19[] = { 0, 0, -0.0171661, 1.38629, 0 };
//	float arr20[] = { 0, 0, 0, 0, 0 };
//	float arr21[] = { 0, 0, 6.83689, 0, 0 };

  vector<float> losses;
  losses.push_back(2.73485);
  losses.push_back(0);
  losses.push_back(3.64118);
  losses.push_back(1.47347);
//	losses.push_back(3.64118);
//	losses.push_back(4.16278);
//	losses.push_back(3.13952);
  losses.push_back(0);
  losses.push_back(2.73485);
//	losses.push_back(1.47347);
//	losses.push_back(3.64118);
//	losses.push_back(3.64118);
//	losses.push_back(2.51662);
//	losses.push_back(2.73485);
  losses.push_back(2.73485);
  losses.push_back(3.64118);
//	losses.push_back(3.64118);
//	losses.push_back(4.16278);
//	losses.push_back(1.47347);
//	losses.push_back(0);
//	losses.push_back(3.13952);

  vector<float> vec1(arr1,arr1+5);
  vector<float> vec2(arr2,arr2+5);
  vector<float> vec3(arr3,arr3+5);
  vector<float> vec4(arr4,arr4+5);
//	vector<float> vec5(arr5,arr5+5);
//	vector<float> vec6(arr6,arr6+5);
//	vector<float> vec7(arr7,arr7+5);
  vector<float> vec8(arr8,arr8+5);
  vector<float> vec9(arr9,arr9+5);
//	vector<float> vec10(arr10,arr10+5);
//	vector<float> vec11(arr11,arr11+5);
//	vector<float> vec12(arr12,arr12+5);
//	vector<float> vec13(arr13,arr13+5);
//	vector<float> vec14(arr14,arr14+5);
  vector<float> vec15(arr15,arr15+5);
  vector<float> vec16(arr16,arr16+5);
//	vector<float> vec17(arr17,arr17+5);
//	vector<float> vec18(arr18,arr18+5);
//	vector<float> vec19(arr19,arr19+5);
//	vector<float> vec20(arr20,arr20+5);
//	vector<float> vec21(arr21,arr21+5);

  s1.PlusEquals(&multi,vec1);
  s2.PlusEquals(&multi,vec2);
  s3.PlusEquals(&multi,vec3);
  s4.PlusEquals(&multi,vec4);
//	s5.PlusEquals(&multi,vec5);
//	s6.PlusEquals(&multi,vec6);
//	s7.PlusEquals(&multi,vec7);
  s8.PlusEquals(&multi,vec8);
  s9.PlusEquals(&multi,vec9);
//	s10.PlusEquals(&multi,vec10);
//	s11.PlusEquals(&multi,vec11);
//	s12.PlusEquals(&multi,vec12);
//	s13.PlusEquals(&multi,vec13);
//	s14.PlusEquals(&multi,vec14);
  s15.PlusEquals(&multi,vec15);
  s16.PlusEquals(&multi,vec16);
//	s17.PlusEquals(&multi,vec17);
//	s18.PlusEquals(&multi,vec18);
//	s19.PlusEquals(&multi,vec19);
//	s20.PlusEquals(&multi,vec20);
//	s21.PlusEquals(&multi,vec21);

  featureValueDiffs.push_back(s1);
  featureValueDiffs.push_back(s2);
  featureValueDiffs.push_back(s3);
  featureValueDiffs.push_back(s4);
//	featureValueDiffs.push_back(s5);
//	featureValueDiffs.push_back(s6);
//	featureValueDiffs.push_back(s7);
  featureValueDiffs.push_back(s8);
  featureValueDiffs.push_back(s9);
//	featureValueDiffs.push_back(s10);
//	featureValueDiffs.push_back(s11);
//	featureValueDiffs.push_back(s12);
//	featureValueDiffs.push_back(s13);
//	featureValueDiffs.push_back(s14);
  featureValueDiffs.push_back(s15);
  featureValueDiffs.push_back(s16);
//	featureValueDiffs.push_back(s17);
//	featureValueDiffs.push_back(s18);
//	featureValueDiffs.push_back(s19);
//	featureValueDiffs.push_back(s20);
//	featureValueDiffs.push_back(s21);

  vector<float> oldModelScoreDiff;
  for (int i = 0; i < numberOfConstraints; ++i) {
    oldModelScoreDiff.push_back(featureValueDiffs[i].InnerProduct(weights));
  }

  for (int i = 0; i < numberOfConstraints; ++i) {
    cerr << "old model score diff: " << oldModelScoreDiff[i] << ", loss: " << losses[i] << "\t" << (oldModelScoreDiff[i] >= losses[i] ? 1 : 0) << endl;
  }

  for (int i = 0; i < numberOfConstraints; ++i) {
    lossMinusModelScoreDiff.push_back(losses[i] - oldModelScoreDiff[i]);
  }

  for (int i = 0; i < numberOfConstraints; ++i) {
    cerr << "A: " << featureValueDiffs[i] << ", b: " << lossMinusModelScoreDiff[i] << endl;
  }

  vector< float> alphas1 = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiff);
  vector< float> alphas2 = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiff, 0.01);

  cerr << "\nalphas without slack:" << endl;
  for (size_t i = 0; i < alphas1.size(); ++i) {
    cerr << "alpha " << i << ": " << alphas1[i] << endl;
  }
  cerr << endl;

  cerr << "partial updates:" << endl;
  vector< ScoreComponentCollection> featureValueDiffs1(featureValueDiffs);
  FVector totalUpdate1 = ScoreComponentCollection::CreateFVector();
  for (size_t k = 0; k < featureValueDiffs1.size(); ++k) {
    featureValueDiffs1[k].MultiplyEquals(alphas1[k]);
    cerr << k << ": " << featureValueDiffs1[k].GetScoresVector() << endl;
    FVector update = featureValueDiffs1[k].GetScoresVector();
    totalUpdate1 += update;
  }
  cerr << endl;
  cerr << "total update: " << totalUpdate1 << endl << endl;

  ScoreComponentCollection weightsUpdate1(weights);
  weightsUpdate1.PlusEquals(totalUpdate1);
  cerr << "old weights: " << weights << endl;
  cerr << "new weights: " << weightsUpdate1 << endl << endl;

  vector<float> newModelScoreDiff;
  for (int i = 0; i < numberOfConstraints; ++i) {
    newModelScoreDiff.push_back(featureValueDiffs[i].InnerProduct(weightsUpdate1));
  }

  for (int i = 0; i < numberOfConstraints; ++i) {
    cerr << "new model score diff: " << newModelScoreDiff[i] << ", loss: " << losses[i] << "\t" << (newModelScoreDiff[i] >= losses[i] ? 1 : 0) << endl;
  }

  cerr << "\n\nalphas with slack 0.01:" << endl;
  for (size_t i = 0; i < alphas2.size(); ++i) {
    cerr << "alpha " << i << ": " << alphas2[i] << endl;
  }
  cerr << endl;

  cerr << "partial updates:" << endl;
  vector< ScoreComponentCollection> featureValueDiffs2(featureValueDiffs);
  FVector totalUpdate2 = ScoreComponentCollection::CreateFVector();
  for (size_t k = 0; k < featureValueDiffs2.size(); ++k) {
    featureValueDiffs2[k].MultiplyEquals(alphas2[k]);
    cerr << k << ": " << featureValueDiffs2[k].GetScoresVector() << endl;
    FVector update = featureValueDiffs2[k].GetScoresVector();
    totalUpdate2 += update;
  }
  cerr << endl;
  cerr << "total update: " << totalUpdate2 << endl << endl;

  ScoreComponentCollection weightsUpdate2(weights);
  weightsUpdate2.PlusEquals(totalUpdate2);
  cerr << "old weights: " << weights << endl;
  cerr << "new weights: " << weightsUpdate2 << endl << endl;

  newModelScoreDiff.clear();
  for (int i = 0; i < numberOfConstraints; ++i) {
    newModelScoreDiff.push_back(featureValueDiffs[i].InnerProduct(weightsUpdate2));
  }

  for (int i = 0; i < numberOfConstraints; ++i) {
    cerr << "new model score diff: " << newModelScoreDiff[i] << ", loss: " << losses[i] << endl;
  }
}

BOOST_FIXTURE_TEST_CASE(test_hildreth_5, MockProducers)
{
  // Unfeasible example with 2 constraints
  cerr << "\n>>>>>Hildreth test, without slack and with 0.01 slack" << endl << endl;
  vector< ScoreComponentCollection> featureValueDiffs;
  vector< float> lossMinusModelScoreDiff;

  // initial weights
  float w[] = { 1, 1, 0.638672, 1, 0 };
  vector<float> vec(w,w+5);
  ScoreComponentCollection weights;
  weights.PlusEquals(&multi, vec);

  int numberOfConstraints = 2;

  // feature value differences (to oracle)
  // NOTE: these feature values are only approximations
  ScoreComponentCollection s1, s17;
  float arr1[] = { 0, 0, -2.0672, 0, 0 };
  float arr17[] = { 0, 0, 4.4283, 0, 0 };
  vector<float> losses;
  losses.push_back(2.73485);
  losses.push_back(3.64118);

  vector<float> vec1(arr1,arr1+5);
  vector<float> vec17(arr17,arr17+5);

  s1.PlusEquals(&multi,vec1);
  s17.PlusEquals(&multi,vec17);

  featureValueDiffs.push_back(s1);
  featureValueDiffs.push_back(s17);

  vector<float> oldModelScoreDiff;
  for (int i = 0; i < numberOfConstraints; ++i) {
    oldModelScoreDiff.push_back(featureValueDiffs[i].InnerProduct(weights));
  }

  float sumOfOldError = 0;
  for (int i = 0; i < numberOfConstraints; ++i) {
    cerr << "old model score diff: " << oldModelScoreDiff[i] << ", loss: " << losses[i] << "\t" << (oldModelScoreDiff[i] >= losses[i] ? 1 : 0) << endl;
    sumOfOldError += (losses[i] - oldModelScoreDiff[i]);
  }
  cerr << "sum of old error: " << sumOfOldError << endl;

  for (int i = 0; i < numberOfConstraints; ++i) {
    lossMinusModelScoreDiff.push_back(losses[i] - oldModelScoreDiff[i]);
  }

  for (int i = 0; i < numberOfConstraints; ++i) {
    cerr << "A: " << featureValueDiffs[i] << ", b: " << lossMinusModelScoreDiff[i] << endl;
  }

  vector< float> alphas1 = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiff);
  vector< float> alphas2 = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiff, 0.01);
  vector< float> alphas3 = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiff, 0.1);

  cerr << "\nalphas without slack:" << endl;
  for (size_t i = 0; i < alphas1.size(); ++i) {
    cerr << "alpha " << i << ": " << alphas1[i] << endl;
  }
  cerr << endl;

  cerr << "partial updates:" << endl;
  vector< ScoreComponentCollection> featureValueDiffs1(featureValueDiffs);
  FVector totalUpdate1 = ScoreComponentCollection::CreateFVector();
  for (size_t k = 0; k < featureValueDiffs1.size(); ++k) {
    featureValueDiffs1[k].MultiplyEquals(alphas1[k]);
    cerr << k << ": " << featureValueDiffs1[k].GetScoresVector() << endl;
    FVector update = featureValueDiffs1[k].GetScoresVector();
    totalUpdate1 += update;
  }
  cerr << endl;
  cerr << "total update: " << totalUpdate1 << endl << endl;

  ScoreComponentCollection weightsUpdate1(weights);
  weightsUpdate1.PlusEquals(totalUpdate1);
  cerr << "old weights: " << weights << endl;
  cerr << "new weights: " << weightsUpdate1 << endl << endl;

  vector<float> newModelScoreDiff;
  for (int i = 0; i < numberOfConstraints; ++i) {
    newModelScoreDiff.push_back(featureValueDiffs[i].InnerProduct(weightsUpdate1));
  }

  float sumOfNewError = 0;
  for (int i = 0; i < numberOfConstraints; ++i) {
    cerr << "new model score diff: " << newModelScoreDiff[i] << ", loss: " << losses[i] << "\t" << (newModelScoreDiff[i] >= losses[i] ? 1 : 0) << endl;
    sumOfNewError += (losses[i] - newModelScoreDiff[i]);
  }
  cerr << "sum of new error: " << sumOfNewError << endl;

  cerr << "\n\nalphas with slack 0.01:" << endl;
  for (size_t i = 0; i < alphas2.size(); ++i) {
    cerr << "alpha " << i << ": " << alphas2[i] << endl;
  }
  cerr << endl;

  cerr << "partial updates:" << endl;
  vector< ScoreComponentCollection> featureValueDiffs2(featureValueDiffs);
  FVector totalUpdate2 = ScoreComponentCollection::CreateFVector();
  for (size_t k = 0; k < featureValueDiffs2.size(); ++k) {
    featureValueDiffs2[k].MultiplyEquals(alphas2[k]);
    cerr << k << ": " << featureValueDiffs2[k].GetScoresVector() << endl;
    FVector update = featureValueDiffs2[k].GetScoresVector();
    totalUpdate2 += update;
  }
  cerr << endl;
  cerr << "total update: " << totalUpdate2 << endl << endl;

  ScoreComponentCollection weightsUpdate2(weights);
  weightsUpdate2.PlusEquals(totalUpdate2);
  cerr << "old weights: " << weights << endl;
  cerr << "new weights: " << weightsUpdate2 << endl << endl;

  newModelScoreDiff.clear();
  for (int i = 0; i < numberOfConstraints; ++i) {
    newModelScoreDiff.push_back(featureValueDiffs[i].InnerProduct(weightsUpdate2));
  }

  sumOfNewError = 0;
  for (int i = 0; i < numberOfConstraints; ++i) {
    cerr << "new model score diff: " << newModelScoreDiff[i] << ", loss: " << losses[i] << "\t" << (newModelScoreDiff[i] >= losses[i] ? 1 : 0) << endl;
    sumOfNewError += (losses[i] - newModelScoreDiff[i]);
  }
  cerr << "sum of new error: " << sumOfNewError << endl;

  cerr << "\n\nalphas with slack 0.1:" << endl;
  for (size_t i = 0; i < alphas3.size(); ++i) {
    cerr << "alpha " << i << ": " << alphas3[i] << endl;
  }
  cerr << endl;

  cerr << "partial updates:" << endl;
  vector< ScoreComponentCollection> featureValueDiffs3(featureValueDiffs);
  FVector totalUpdate3 = ScoreComponentCollection::CreateFVector();
  for (size_t k = 0; k < featureValueDiffs3.size(); ++k) {
    featureValueDiffs3[k].MultiplyEquals(alphas3[k]);
    cerr << k << ": " << featureValueDiffs3[k].GetScoresVector() << endl;
    FVector update = featureValueDiffs3[k].GetScoresVector();
    totalUpdate3 += update;
  }
  cerr << endl;
  cerr << "total update: " << totalUpdate3 << endl << endl;

  ScoreComponentCollection weightsUpdate3(weights);
  weightsUpdate3.PlusEquals(totalUpdate3);
  cerr << "old weights: " << weights << endl;
  cerr << "new weights: " << weightsUpdate3 << endl << endl;

  newModelScoreDiff.clear();
  for (int i = 0; i < numberOfConstraints; ++i) {
    newModelScoreDiff.push_back(featureValueDiffs[i].InnerProduct(weightsUpdate3));
  }

  sumOfNewError = 0;
  for (int i = 0; i < numberOfConstraints; ++i) {
    cerr << "new model score diff: " << newModelScoreDiff[i] << ", loss: " << losses[i] << "\t" << (newModelScoreDiff[i] >= losses[i] ? 1 : 0) << endl;
    sumOfNewError += (losses[i] - newModelScoreDiff[i]);
  }
  cerr << "sum of new error: " << sumOfNewError << endl;
}

BOOST_AUTO_TEST_SUITE_END()

}

