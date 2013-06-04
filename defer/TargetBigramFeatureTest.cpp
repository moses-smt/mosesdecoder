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
#include <unistd.h>

#include <boost/test/unit_test.hpp>

#include "FactorCollection.h"
#include "Sentence.h"
#include "TargetBigramFeature.h"
#include "Word.h"

#include "MockHypothesis.h"

using namespace std;
using namespace Moses;

namespace MosesTest
{

BOOST_AUTO_TEST_SUITE(target_bigram)

static Word MakeWord(string text)
{
  FactorCollection &factorCollection = FactorCollection::Instance();
  const Factor* f = factorCollection.AddFactor(Input,0,text);
  Word w;
  w.SetFactor(0,f);
  return w;
}

class VocabFileFixture
{
public:
  template<class I>
  VocabFileFixture(I begin, I end) {
    char name[] = "TargetBigramXXXXXX";
    int fd = mkstemp(name);
    BOOST_CHECK(fd != -1);
    BOOST_CHECK(!close(fd));
    filename = name;
    ofstream out(name);
    for (I i = begin; i != end; ++i) {
      out << *i << endl;
    }
    out.close();
  }

  ~VocabFileFixture() {
    BOOST_CHECK(!remove(filename.c_str()));
  }

  string filename;
};

/*
BOOST_AUTO_TEST_CASE(Test2)
{
  HypothesisFixture hypos;
  cerr << hypos.empty() << ", " << *hypos.empty() << endl;
  cerr << hypos.partial() << ", " <<  *hypos.partial() << endl;
  cerr << hypos.full() << ", " << *hypos.full() << endl;
  BOOST_CHECK(true);
} */

BOOST_AUTO_TEST_CASE(state_compare)
{
  Word w1 = MakeWord("w1");
  Word w2 = MakeWord("w2");
  TargetBigramState s1(w1);
  TargetBigramState s2(w2);
  BOOST_CHECK_EQUAL(s1.Compare(s1),0);
  BOOST_CHECK_EQUAL(s2.Compare(s2),0);
  BOOST_CHECK_NE(s1.Compare(s2),0);
  BOOST_CHECK_NE(s2.Compare(s1),0);

}

BOOST_AUTO_TEST_CASE(load)
{
  TargetBigramFeature tbf;
  string vocab[] = {"je", "ne", "pas"};
  VocabFileFixture vocabFixture(vocab,vocab+3);
  BOOST_CHECK(tbf.Load(vocabFixture.filename));
  BOOST_CHECK(!tbf.Load("/gweugegyiegy"));
}

BOOST_AUTO_TEST_CASE(score_components)
{
  TargetBigramFeature tbf;
  BOOST_CHECK_EQUAL(
    tbf.GetNumScoreComponents(),
    ScoreProducer::unlimited);
}

BOOST_AUTO_TEST_CASE(empty_hypo)
{
  Sentence s;
  TargetBigramFeature tbf;
  auto_ptr<const FFState> ffs(tbf.EmptyHypothesisState(s));
  BOOST_CHECK(ffs.get());
  TargetBigramState expected(MakeWord(BOS_));
  BOOST_CHECK_EQUAL(ffs->Compare(expected),0);
}

//Test of evaluate() where a vocab is specified
BOOST_AUTO_TEST_CASE(evaluate_vocab)
{
  string vocab[] = {"i", "do"};
  VocabFileFixture vocabFile(vocab,vocab+2);
  HypothesisFixture hypos;
  TargetBigramFeature tbf;
  BOOST_CHECK(tbf.Load(vocabFile.filename));
  TargetBigramState prevState(MakeWord("i"));
  ScoreComponentCollection scc;
  auto_ptr<FFState> currState(
    tbf.Evaluate(*hypos.partial(), &prevState, &scc));

  BOOST_CHECK_EQUAL(scc.GetScoreForProducer(&tbf, "i:do"),1);
  BOOST_CHECK_EQUAL(scc.GetScoreForProducer(&tbf, "do:not"),0);
  BOOST_CHECK(! currState->Compare(TargetBigramState(MakeWord("not"))));
}

//Test of evaluate() where no vocab file is specified
BOOST_AUTO_TEST_CASE(evaluate_all)
{
  HypothesisFixture hypos;
  TargetBigramFeature tbf;
  BOOST_CHECK(tbf.Load("*"));
  TargetBigramState prevState(MakeWord("i"));
  ScoreComponentCollection scc;
  auto_ptr<FFState> currState(
    tbf.Evaluate(*hypos.partial(),&prevState,&scc));

  BOOST_CHECK_EQUAL(scc.GetScoreForProducer(&tbf, "i:do"),1);
  BOOST_CHECK_EQUAL(scc.GetScoreForProducer(&tbf, "do:not"),1);
  BOOST_CHECK_EQUAL(scc.GetScoreForProducer(&tbf, "not:</s>"),0);
  BOOST_CHECK(! currState->Compare(TargetBigramState(MakeWord("not"))));

}

BOOST_AUTO_TEST_CASE(evaluate_empty)
{
  HypothesisFixture hypos;
  TargetBigramFeature tbf;
  BOOST_CHECK(tbf.Load("*"));
  auto_ptr<const FFState> prevState(tbf.EmptyHypothesisState(Sentence()));
  ScoreComponentCollection scc;
  auto_ptr<const FFState> currState(
    tbf.Evaluate(*hypos.empty(),prevState.get(),&scc));
  BOOST_CHECK(! currState->Compare(*prevState));
}

BOOST_AUTO_TEST_CASE(evaluate_eos)
{
  HypothesisFixture hypos;
  TargetBigramFeature tbf;
  BOOST_CHECK(tbf.Load("*"));
  TargetBigramState prevState(MakeWord("know"));
  ScoreComponentCollection scc;
  auto_ptr<const FFState> currState(
    tbf.Evaluate(*hypos.full(),&prevState,&scc));

  BOOST_CHECK_EQUAL(scc.GetScoreForProducer(&tbf, ".:</s>"),1);
  BOOST_CHECK_EQUAL(scc.GetScoreForProducer(&tbf, "know:."),1);

  BOOST_CHECK(! currState.get());
}

BOOST_AUTO_TEST_SUITE_END()

}

