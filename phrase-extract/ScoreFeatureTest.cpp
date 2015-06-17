/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2012- University of Edinburgh

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

#include "DomainFeature.h"
#include "ScoreFeature.h"
#include "tables-core.h"

#define  BOOST_TEST_MODULE MosesTrainingScoreFeature
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <unordered_set>

using namespace MosesTraining;
using namespace std;

//pesky global variables
namespace MosesTraining
{
bool hierarchicalFlag = false;
Vocabulary vcbT;
Vocabulary vcbS;
}


const char *DomainFileLocation()
{
  if (boost::unit_test::framework::master_test_suite().argc < 2) {
    return "test.domain";
  }
  return boost::unit_test::framework::master_test_suite().argv[1];
}


BOOST_AUTO_TEST_CASE(manager_configure_domain_except)
{
  //Check that configure rejects illegal domain arg combinations
  ScoreFeatureManager manager;
  BOOST_CHECK_THROW(
    manager.configure( {"--DomainRatio","/dev/null","--DomainIndicator","/dev/null"}),
    ScoreFeatureArgumentException);
  BOOST_CHECK_THROW(
    manager.configure( {"--SparseDomainSubset","/dev/null","--SparseDomainRatio","/dev/null"}),
    ScoreFeatureArgumentException);
  BOOST_CHECK_THROW(
    manager.configure( {"--SparseDomainBlah","/dev/null"}),
    ScoreFeatureArgumentException);
  BOOST_CHECK_THROW(
    manager.configure( {"--DomainSubset"}),
    ScoreFeatureArgumentException);
}

template <class Expected>
static void checkDomainConfigured(
  const vector<string>& args)
{
  ScoreFeatureManager manager;
  manager.configure(args);
  const std::vector<ScoreFeaturePtr>& features  = manager.getFeatures();
  //BOOST_REQUIRE_EQUAL(features.size(), 2);
  //if I add to features this check will fail?
  BOOST_REQUIRE_EQUAL(features.size(), 1); //MARIA -> what is this check and why does it fail when I add my feature?
  Expected* feature = dynamic_cast<Expected*>(features[0].get());
  BOOST_REQUIRE(feature);
  BOOST_CHECK(manager.includeSentenceId());
}

BOOST_AUTO_TEST_CASE(manager_config_domain)
{
  checkDomainConfigured<RatioDomainFeature>
  ( {"--DomainRatio","/dev/null"});
  checkDomainConfigured<IndicatorDomainFeature>
  ( {"--DomainIndicator","/dev/null"});
  checkDomainConfigured<SubsetDomainFeature>
  ( {"--DomainSubset","/dev/null"});
  checkDomainConfigured<SparseRatioDomainFeature>
  ( {"--SparseDomainRatio","/dev/null"});
  checkDomainConfigured<SparseIndicatorDomainFeature>
  ( {"--SparseDomainIndicator","/dev/null"});
  checkDomainConfigured<SparseSubsetDomainFeature>
  ( {"--SparseDomainSubset","/dev/null"});

  unordered_set<int> s;
  s.insert(4);
  s.insert(7);
  s.insert(4);
  s.insert(1);

for (auto i: s) {
    cerr << i << " ";
  }
}

