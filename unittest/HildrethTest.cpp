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

BOOST_AUTO_TEST_SUITE(hildreth_test)

BOOST_FIXTURE_TEST_CASE(test_return_values, MockProducers)
{
	cerr << "Hildreth test" << endl;
	vector< ScoreComponentCollection> featureValueDiffs;
	vector< float> lossMarginDistances;

	ScoreComponentCollection s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15, s16, s17, s18, s19, s20, s21, s22, s23, s24, s25, s26, s27, s28, s29, s30;
	float arr1[] = {0, 0, -0.0191922, 0, 0, };
	float arr2[] = {0, 0, 0, 0, 0, };
	float arr3[] = {0, 0, -0.0179048, 0.81268, 0, };
	float arr4[] = {0, 0, 0.00127029, 0.81268, 0, };
	float arr5[] = {0, 0, -0.0191922, 1.09404, 0, };
	float arr6[] = {0, 0, -0.0179048, 1.09404, 0, };
	float arr7[] = {0, 0, 0, 1.09404, 0, };
	float arr8[] = {0, 0, 0.00127029, 1.09404, 0, };
	float arr9[] = {0, 0, 0.383343, 0, 0, };
	float arr10[] = {0, 0, 0.388091, 0, 0, };
	float arr11[] = {0, 0, 0, 0, 0, };
	float arr12[] = {0, 0, 0, 1.09404, 0, };
	float arr13[] = {0, 0, -0.0191922, 0, 0, };
	float arr14[] = {0, 0, 0.00127029, 0.81268, 0, };
	float arr15[] = {0, 0, -0.0191922, 1.09404, 0, };
	float arr16[] = {0, 0, 0.00127029, 1.09404, 0, };
	float arr17[] = {0, 0, -0.0179048, 0.81268, 0, };
	float arr18[] = {0, 0, -0.0179048, 1.09404, 0, };
	float arr19[] = {0, 0, 0.383343, 0, 0, };
	float arr20[] = {0, 0, 0.527225, 0.462822, 0, };
	float arr21[] = {0, 0, 0.927927, 0.462822, 0, };
	float arr22[] = {0, 0, 0.8653, 1.09404, 0, };
	float arr23[] = {2.32193, 0, 0.832051, 0.462822, 0, };
	float arr24[] = {1.58496, 0, 0.913224, 0.462822, 0, };
	float arr25[] = {1.58496, 0, 0.849941, 1.09404, 0, };
	float arr26[] = {2.58496, 0, 0.832051, 0.462822, 0, };
	float arr27[] = {2, 0, 0.913224, 0.462822, 0, };
	float arr28[] = {2, 0, 0.927927, 0.462822, 0, };
	float arr29[] = {2, 0, 0.920873, 1.09404, 0, };
	float arr30[] = {2, 0, 0.935499, 1.09404, 0, };
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
	vector<float> vec22(arr22,arr22+5);
	vector<float> vec23(arr23,arr23+5);
	vector<float> vec24(arr24,arr24+5);
	vector<float> vec25(arr25,arr25+5);
	vector<float> vec26(arr26,arr26+5);
	vector<float> vec27(arr27,arr27+5);
	vector<float> vec28(arr28,arr28+5);
	vector<float> vec29(arr29,arr29+5);
	vector<float> vec30(arr30,arr30+5);
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
	s22.PlusEquals(&multi,vec22);
	s23.PlusEquals(&multi,vec23);
	s24.PlusEquals(&multi,vec24);
	s25.PlusEquals(&multi,vec25);
	s26.PlusEquals(&multi,vec26);
	s27.PlusEquals(&multi,vec27);
	s28.PlusEquals(&multi,vec28);
	s29.PlusEquals(&multi,vec29);
	s30.PlusEquals(&multi,vec20);
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
	featureValueDiffs.push_back(s22);
	featureValueDiffs.push_back(s23);
	featureValueDiffs.push_back(s24);
	featureValueDiffs.push_back(s25);
	featureValueDiffs.push_back(s26);
	featureValueDiffs.push_back(s27);
	featureValueDiffs.push_back(s28);
	featureValueDiffs.push_back(s29);
	featureValueDiffs.push_back(s30);

	lossMarginDistances.push_back(0.151855);
	lossMarginDistances.push_back(0);
	lossMarginDistances.push_back(0.0451589);
	lossMarginDistances.push_back(-0.0484961);
	lossMarginDistances.push_back(-0.112154);
	lossMarginDistances.push_back(-0.0227373);
	lossMarginDistances.push_back(-0.264009);
	lossMarginDistances.push_back(-0.116392);
	lossMarginDistances.push_back(0.135853);
	lossMarginDistances.push_back(0.157176);
	lossMarginDistances.push_back(0);
	lossMarginDistances.push_back(-0.264009);
	lossMarginDistances.push_back(0.151855);
	lossMarginDistances.push_back(-0.0484961);
	lossMarginDistances.push_back(-0.112154);
	lossMarginDistances.push_back(-0.116392);
	lossMarginDistances.push_back(0.0451589);
	lossMarginDistances.push_back(-0.0227373);
	lossMarginDistances.push_back(0.135853);
	lossMarginDistances.push_back(-0.0730133);
	lossMarginDistances.push_back(0.546107);
	lossMarginDistances.push_back(0.406757);
	lossMarginDistances.push_back(0.139081);
	lossMarginDistances.push_back(0.257758);
	lossMarginDistances.push_back(0.118544);
	lossMarginDistances.push_back(0.0907223);
	lossMarginDistances.push_back(0.181454);
	lossMarginDistances.push_back(0.178408);
	lossMarginDistances.push_back(0.027547);
	lossMarginDistances.push_back(0.0245174);

	vector< float> alphas1 = Hildreth::optimise(featureValueDiffs, lossMarginDistances);
	vector< float> alphas2 = Hildreth::optimise(featureValueDiffs, lossMarginDistances, 0.01);

	cerr << "Alphas without slack:" << endl;
	for (size_t i = 0; i < alphas1.size(); ++i) {
		cerr << "alpha " << i << ": " << alphas1[i] << endl;
	}
	cerr << endl;

	cerr << "Update without slack:" << endl;
	vector< ScoreComponentCollection> featureValueDiffs1(featureValueDiffs);
	FVector totalUpdate1(0);
	for (size_t k = 0; k < featureValueDiffs1.size(); ++k) {
		featureValueDiffs1[k].MultiplyEquals(alphas1[k]);
		cerr << k << ": " << featureValueDiffs1[k].GetScoresVector() << endl;
		FVector update = featureValueDiffs1[k].GetScoresVector();
		totalUpdate1 += update;
	}
	cerr << endl;
	cerr << "total update: " << totalUpdate1 << endl << endl;

	cerr << "Alphas with slack 0.01:" << endl;
	for (size_t i = 0; i < alphas2.size(); ++i) {
		cerr << "alpha " << i << ": " << alphas2[i] << endl;
	}

	cerr << "Update with slack 0.01:" << endl;
	vector< ScoreComponentCollection> featureValueDiffs2(featureValueDiffs);
	FVector totalUpdate2(0);
	for (size_t k = 0; k < featureValueDiffs2.size(); ++k) {
		featureValueDiffs2[k].MultiplyEquals(alphas2[k]);
		cerr << k << ": " << featureValueDiffs2[k].GetScoresVector() << endl;
		FVector update = featureValueDiffs2[k].GetScoresVector();
		totalUpdate2 += update;
	}
	cerr << endl;
	cerr << "total update: " << totalUpdate2 << endl << endl;
}


BOOST_AUTO_TEST_SUITE_END()

}

