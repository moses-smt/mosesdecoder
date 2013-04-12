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
#define BOOST_TEST_MODULE BackwardTest
#include <boost/test/unit_test.hpp>

#include "lm/config.hh"
#include "lm/left.hh"
#include "lm/model.hh"
#include "lm/state.hh"

#include "moses/Sentence.h"
#include "moses/TypeDef.h"

#include "moses/StaticData.h"

//#include "BackwardLMState.h"
#include "moses/LM/Backward.h"
#include "moses/LM/BackwardLMState.h"
#include "moses/Util.h"

#include "lm/state.hh"
#include "lm/left.hh"

#include <vector>

using namespace Moses;
//using namespace std;
/*
template <class M> void Foo() {


  Moses::BackwardLanguageModel<M> *backwardLM;
  // = new Moses::BackwardLanguageModel<M>( filename, factorType, lazy );
  

}
template <class M> void Everything() {
  //  Foo<M>();
}
*/

namespace Moses {

// Apparently some Boost versions use templates and are pretty strict about types matching.  
#define SLOPPY_CHECK_CLOSE(ref, value, tol) BOOST_CHECK_CLOSE(static_cast<double>(ref), static_cast<double>(value), static_cast<double>(tol));

class BackwardLanguageModelTest {

  public:
    BackwardLanguageModelTest() : 
      dummyInput(new Sentence()),
      backwardLM(
	       //           BackwardLanguageModel
		 //		 new Moses::BackwardLanguageModel<Model>(
	      ConstructBackwardLM( 
		  		     boost::unit_test::framework::master_test_suite().argv[1], 
		 		     0,
		 		     false)
		 )
    {
      // This space intentionally left blank
    }

    ~BackwardLanguageModelTest() {
      delete dummyInput;
      delete backwardLM;
    }

    void testEmptyHypothesis() {
      FFState *ffState = const_cast< FFState * >(backwardLM->EmptyHypothesisState( *dummyInput ));

      BOOST_CHECK( ffState != NULL );

      delete ffState;
    }

    void testCalcScore() {

      double p_the      = -1.383059;
      double p_licenses = -2.360783;
      double p_for      = -1.661813;
      double p_most     = -2.360783;
      double p_software = -1.62042;

      double p_the_licenses  = -0.9625873;
      double p_licenses_for  = -1.661557;
      double p_for_most      = -0.4526253;
      double p_most_software = -1.70295; 

      double p_the_licenses_for  = p_the_licenses + p_licenses_for;
      double p_licenses_for_most = p_licenses_for + p_for_most;
 
      // the
      {
	Phrase phrase;
	BOOST_CHECK( phrase.GetSize() == 0 );

	std::vector<FactorType> outputFactorOrder;
	outputFactorOrder.push_back(0);

	phrase.CreateFromString(
				outputFactorOrder,
				"the", 
				StaticData::Instance().GetFactorDelimiter());

	BOOST_CHECK( phrase.GetSize() == 1 );
      
	float fullScore;
	float ngramScore;
	size_t oovCount;
	backwardLM->CalcScore(phrase, fullScore, ngramScore, oovCount);

	BOOST_CHECK( oovCount == 0 );
	SLOPPY_CHECK_CLOSE( TransformLMScore(p_the), fullScore, 0.01);
	SLOPPY_CHECK_CLOSE( TransformLMScore( 0.0 ), ngramScore, 0.01);
      }

      // the licenses
      {
	Phrase phrase;
	BOOST_CHECK( phrase.GetSize() == 0 );

	std::vector<FactorType> outputFactorOrder;
	outputFactorOrder.push_back(0);

	phrase.CreateFromString(
				outputFactorOrder,
				"the licenses", 
				StaticData::Instance().GetFactorDelimiter());

	BOOST_CHECK( phrase.GetSize() == 2 );
      
	float fullScore;
	float ngramScore;
	size_t oovCount;
	backwardLM->CalcScore(phrase, fullScore, ngramScore, oovCount);

	BOOST_CHECK( oovCount == 0 );
	SLOPPY_CHECK_CLOSE( TransformLMScore(p_licenses + p_the_licenses), fullScore, 0.01);
	SLOPPY_CHECK_CLOSE( TransformLMScore( 0.0 ), ngramScore, 0.01);
      }
      
      // the licenses for
      {
	Phrase phrase;
	BOOST_CHECK( phrase.GetSize() == 0 );

	std::vector<FactorType> outputFactorOrder;
	outputFactorOrder.push_back(0);

	phrase.CreateFromString(
				outputFactorOrder,
				"the licenses for", 
				StaticData::Instance().GetFactorDelimiter());

	BOOST_CHECK( phrase.GetSize() == 3 );
      
	float fullScore;
	float ngramScore;
	size_t oovCount;
	backwardLM->CalcScore(phrase, fullScore, ngramScore, oovCount);

	BOOST_CHECK( oovCount == 0 );
	SLOPPY_CHECK_CLOSE( TransformLMScore( p_the_licenses_for ), ngramScore, 0.01);
	SLOPPY_CHECK_CLOSE( TransformLMScore(p_for + p_licenses_for + p_the_licenses), fullScore, 0.01);
      }
     
      // the licenses for most
      {
	Phrase phrase;
	BOOST_CHECK( phrase.GetSize() == 0 );

	std::vector<FactorType> outputFactorOrder;
	outputFactorOrder.push_back(0);

	phrase.CreateFromString(
				outputFactorOrder,
				"the licenses for most", 
				StaticData::Instance().GetFactorDelimiter());

	BOOST_CHECK( phrase.GetSize() == 4 );
      
	float fullScore;
	float ngramScore;
	size_t oovCount;
	backwardLM->CalcScore(phrase, fullScore, ngramScore, oovCount);

	BOOST_CHECK( oovCount == 0 );
	SLOPPY_CHECK_CLOSE( TransformLMScore( p_the_licenses + p_licenses_for ), ngramScore, 0.01);
	SLOPPY_CHECK_CLOSE( TransformLMScore(p_most + p_for_most + p_licenses_for + p_the_licenses), fullScore, 0.01);
      }
 
    }

  private:
    const Sentence *dummyInput;
    LanguageModel *backwardLM;

};


}

const char *FileLocation() {
  if (boost::unit_test::framework::master_test_suite().argc < 2) {
    BOOST_FAIL("Jamfile must specify arpa file for this test, but did not");
  }
  return boost::unit_test::framework::master_test_suite().argv[1];
}

BOOST_AUTO_TEST_CASE(ProbingAll) {

  BackwardLanguageModelTest test;
  test.testEmptyHypothesis();
  test.testCalcScore();

}
