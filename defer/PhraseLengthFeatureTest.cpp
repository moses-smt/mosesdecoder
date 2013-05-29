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
#include <boost/test/unit_test.hpp>

#include "moses/FF/PhraseLengthFeature.h"
#include "moses/FactorCollection.h"
#include "moses/Sentence.h"
#include "moses/TargetPhrase.h"
#include "moses/TranslationOption.h"

using namespace Moses;
using namespace std;

BOOST_AUTO_TEST_SUITE(phrase_length_feature)

//TODO: Factor out setup code so that it can be reused

static Word MakeWord(string text)
{
  FactorCollection &factorCollection = FactorCollection::Instance();
  const Factor* f = factorCollection.AddFactor(Input,0,text);
  Word w;
  w.SetFactor(0,f);
  return w;
}


BOOST_AUTO_TEST_CASE(evaluate)
{
  Word w1 = MakeWord("w1");
  Word w2 = MakeWord("y2");
  Word w3 = MakeWord("x3");
  Word w4 = MakeWord("w4");

  Phrase p1;
  p1.AddWord(w1);
  p1.AddWord(w3);
  p1.AddWord(w4);

  Phrase p2;
  p2.AddWord(w1);
  p2.AddWord(w2);

  Phrase p3;
  p3.AddWord(w2);
  p3.AddWord(w1);
  p3.AddWord(w4);
  p3.AddWord(w4);

  TargetPhrase tp1(p1);
  TargetPhrase tp2(p2);
  TargetPhrase tp3(p3);

  Sentence sentence;
  vector<FactorType> order;
  order.push_back(0);
  stringstream in("the input sentence has 6 words");
  sentence.Read(in, order);

  TranslationOption topt1(WordsRange(0,0), tp1);
  TranslationOption topt2(WordsRange(1,3), tp2);
  TranslationOption topt3(WordsRange(2,3), tp3);

  PhraseBasedFeatureContext context1(topt1,sentence);
  PhraseBasedFeatureContext context2(topt2,sentence);
  PhraseBasedFeatureContext context3(topt3,sentence);

  PhraseLengthFeature plf;

  ScoreComponentCollection acc1,acc2,acc3;

  plf.Evaluate(context1, &acc1);
  BOOST_CHECK_EQUAL(acc1.GetScoreForProducer(&plf, "s1"),1);
  BOOST_CHECK_EQUAL(acc1.GetScoreForProducer(&plf, "t3"),1);
  BOOST_CHECK_EQUAL(acc1.GetScoreForProducer(&plf, "1,3"),1);

  plf.Evaluate(context2, &acc2);
  BOOST_CHECK_EQUAL(acc2.GetScoreForProducer(&plf, "s3"),1);
  BOOST_CHECK_EQUAL(acc2.GetScoreForProducer(&plf, "t2"),1);
  BOOST_CHECK_EQUAL(acc2.GetScoreForProducer(&plf, "3,2"),1);

  plf.Evaluate(context3, &acc3);
  BOOST_CHECK_EQUAL(acc3.GetScoreForProducer(&plf, "s2"),1);
  BOOST_CHECK_EQUAL(acc3.GetScoreForProducer(&plf, "t4"),1);
  BOOST_CHECK_EQUAL(acc3.GetScoreForProducer(&plf, "2,4"),1);
}

BOOST_AUTO_TEST_SUITE_END()
