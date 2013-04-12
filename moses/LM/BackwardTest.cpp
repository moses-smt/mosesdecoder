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
      /*
      //      lm::ngram::ChartState &state = static_cast< const BackwardLMState >(*ffState).state;
      BackwardLMState *lmState = static_cast< BackwardLMState* >(ffState);
      //const lm::ngram::ChartState &state = static_cast< const BackwardLMState* >(ffState)->state;
      //lm::ngram::ChartState &state = lmState->state;

      //      BOOST_CHECK( state.left.length == 1 );
      //      BOOST_CHECK( state.right.Length() == 0 );

      BackwardLanguageModel<lm::ngram::ProbingModel> *lm = static_cast< BackwardLanguageModel<lm::ngram::ProbingModel> *>(backwardLM);
    lm::ngram::ChartState &state = lmState->state;
    lm::ngram::RuleScore<lm::ngram::ProbingModel> ruleScore(*(lm->m_ngram), state);
    double score = ruleScore.Finish();
    SLOPPY_CHECK_CLOSE(-1.457693, score, 0.001);
      */
      delete ffState;
    }

    void testCalcScore() {
      //std::vector<WordIndex> words

      Phrase phrase;
      BOOST_CHECK( phrase.GetSize() == 0 );

      std::vector<FactorType> outputFactorOrder;
      outputFactorOrder.push_back(0);

      phrase.CreateFromString(
			      //StaticData::Instance().GetOutputFactorOrder(), 
			      outputFactorOrder,
			      "the", 
			      StaticData::Instance().GetFactorDelimiter());

      BOOST_CHECK( phrase.GetSize() == 1 );
      
      // BackwardLanguageModel<lm::ngram::ProbingModel> *lm = static_cast< BackwardLanguageModel<lm::ngram::ProbingModel> *>(backwardLM);

      //Word &word = phrase.GetWord(0);
      //Word
      //      BOOST_CHECK( word == lm->m_ngram->GetVocabulary().Index("the") );
      

      
      float fullScore;
      float ngramScore;
      size_t oovCount;
      backwardLM->CalcScore(phrase, fullScore, ngramScore, oovCount);

      BOOST_CHECK( oovCount == 0 );
      SLOPPY_CHECK_CLOSE( TransformLMScore(-1.383059), fullScore, 0.01);
      SLOPPY_CHECK_CLOSE( TransformLMScore( 0.0 ), ngramScore, 0.01);
      
    }

  private:
    const Sentence *dummyInput;
  //    BackwardLanguageModel<Model> *backwardLM;
    LanguageModel *backwardLM;
  /*
    void LookupVocab(const StringPiece &str, std::vector<WordIndex *> &out) {

      out.clear();
      for (util::TokenIter<util::SingleCharacter, true> i(str, ' '); i; ++i) {
	out.push_back(lm->m_ngram.GetVocabulary().Index(*i));
      }
    }
  */
};


}

const char *FileLocation() {
  if (boost::unit_test::framework::master_test_suite().argc < 2) {
    BOOST_FAIL("Jamfile must specify arpa file for this test, but did not");
  }
  return boost::unit_test::framework::master_test_suite().argv[1];
}

BOOST_AUTO_TEST_CASE(ProbingAll) {
  //  Everything<lm::ngram::Model>();
  /*
  const std::string filename( boost::unit_test::framework::master_test_suite().argv[1] );
  size_t factorType = 0;
  bool lazy = false;

  LanguageModel *backwardLM = ConstructBackwardLM( filename, factorType, lazy );
  const Sentence *dummyInput = new Sentence();

  const FFState *ffState = backwardLM->EmptyHypothesisState( *dummyInput );
  
  //new BackwardLanguageModel<lm::ngram::Model>( filename, factorType, lazy );

  delete dummyInput;
  delete backwardLM;
  */
  //BackwardLanguageModelTest<lm::ngram::TrieModel> test;
BackwardLanguageModelTest test;
  test.testEmptyHypothesis();
  test.testCalcScore();
  //  test->testEmptyHypothesis();
}
