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

#include "FactorCollection.h"
#include "Util.h"

#include "Bleu.h"

using namespace Josiah;
using namespace Moses;
using namespace std;

BOOST_AUTO_TEST_SUITE(bleu)
    
static void checkNgram(const string& ngram, size_t count, const NGramMap& ngrams) {
  Translation t;
  TextToTranslation(ngram,t);
  NGramMap::const_iterator i = ngrams.find(t);
  size_t actualCount = 0;
  if (i != ngrams.end()) {
    actualCount = i->second;
  }
  BOOST_CHECK_MESSAGE(actualCount == count,ngram);
}
    
BOOST_AUTO_TEST_CASE(ref_stats_single) {
  Translation ref,src;
  TextToTranslation("give me the statistics on this sentence , give me",ref);
  TextToTranslation("the source is not really important",src);
  vector<Translation> refs;
  refs.push_back(ref);
  Bleu bleu;
  bleu.AddReferences(refs,src);
  
  NGramMap ngrams = bleu.GetReferenceStats(0);
  BOOST_CHECK_EQUAL(ngrams.size(),(size_t)31);
  checkNgram("give", 2, ngrams);
  checkNgram("me", 2, ngrams);
  checkNgram("the", 1, ngrams);
  checkNgram("statistics", 1, ngrams);
  checkNgram("on", 1, ngrams);
  checkNgram("this", 1, ngrams);
  checkNgram("sentence", 1, ngrams);
  checkNgram(",", 1, ngrams);
  checkNgram("give me", 2, ngrams);
  checkNgram("me the", 1, ngrams);
  checkNgram("the statistics", 1, ngrams);
  checkNgram("statistics on", 1, ngrams);
  checkNgram("on this", 1, ngrams);
  checkNgram("this sentence", 1, ngrams);
  checkNgram("sentence ,", 1, ngrams);
  checkNgram(", give", 1, ngrams);
  checkNgram("give me the", 1, ngrams);
  checkNgram("me the statistics", 1, ngrams);
  checkNgram("the statistics on", 1, ngrams);
  checkNgram("statistics on this", 1, ngrams);
  checkNgram("on this sentence", 1, ngrams);
  checkNgram("sentence , give", 1, ngrams);
  checkNgram(", give me", 1, ngrams);
  checkNgram("give me the statistics", 1, ngrams);
  checkNgram("me the statistics on", 1, ngrams);
  checkNgram("the statistics on this", 1, ngrams);
  checkNgram("statistics on this sentence", 1, ngrams);
  checkNgram("on this sentence ,", 1, ngrams);
  checkNgram("this sentence , give", 1, ngrams);
  checkNgram("sentence , give me", 1, ngrams);
}

BOOST_AUTO_TEST_CASE(ref_stats_multi) {
  Translation ref1,ref2,ref3,src;
  TextToTranslation("what is this saying ?", ref1);
  TextToTranslation("what saying is this ? ?", ref2);
  TextToTranslation("what is this is this ?", ref3);
  TextToTranslation("not important", src);
  vector<Translation> refs;
  refs.push_back(ref1);
  refs.push_back(ref2);
  refs.push_back(ref3);
  Bleu bleu;
  bleu.AddReferences(refs,src);
  NGramMap ngrams = bleu.GetReferenceStats(0);
  BOOST_CHECK_EQUAL(ngrams.size(),(size_t)31);
  checkNgram("what", 1, ngrams);
  checkNgram("is", 2, ngrams);
  checkNgram("this", 2, ngrams);
  checkNgram("saying", 1, ngrams);
  checkNgram("?", 2, ngrams);
  checkNgram("what is", 1, ngrams);
  checkNgram("is this", 2, ngrams);
  checkNgram("this saying", 1, ngrams);
  checkNgram("saying ?", 1, ngrams);
  checkNgram("what saying", 1, ngrams);
  checkNgram("saying is", 1, ngrams);
  checkNgram("this ?", 1, ngrams);
  checkNgram("? ? ", 1, ngrams);
  checkNgram("this is", 1, ngrams);
  checkNgram("what is this", 1, ngrams);
  checkNgram("is this saying", 1, ngrams);
  checkNgram("this saying ?", 1, ngrams);
  checkNgram("what saying is", 1, ngrams);
  checkNgram("saying is this", 1, ngrams);
  checkNgram("is this ?", 1, ngrams);
  checkNgram("this ? ?", 1, ngrams);
  checkNgram("is this is", 1, ngrams);
  checkNgram("this is this", 1, ngrams);
  checkNgram("what is this saying", 1, ngrams);
  checkNgram("is this saying ?", 1, ngrams);
  checkNgram("what saying is this", 1, ngrams);
  checkNgram("saying is this ?", 1, ngrams);
  checkNgram("is this ? ?", 1, ngrams);
  checkNgram("what is this is", 1, ngrams);
  checkNgram("is this is this", 1, ngrams);
  checkNgram("this is this ?", 1, ngrams);
}

BOOST_AUTO_TEST_CASE(ref_length_single) {
  Translation ref,src;
  TextToTranslation("give me the statistics on this sentence , give me",ref);
  TextToTranslation("the source is not really important",src);
  vector<Translation> refs;
  refs.push_back(ref);
  Bleu bleu;
  bleu.AddReferences(refs,src);
  
  vector<size_t> actual = bleu.GetReferenceLengths(0);
  vector<size_t> expected;
  expected.push_back(10);
  BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(),actual.end(),expected.begin(),expected.end());
  
}

BOOST_AUTO_TEST_CASE(ref_length_multi) {
  Translation ref1,ref2,ref3,src;
  TextToTranslation("what is this saying ?", ref1);
  TextToTranslation("what saying is this ? ?", ref2);
  TextToTranslation("what is this is this ?", ref3);
  TextToTranslation("not important", src);
  vector<Translation> refs;
  refs.push_back(ref1);
  refs.push_back(ref2);
  refs.push_back(ref3);
  Bleu bleu;
  bleu.AddReferences(refs,src);
  
  vector<size_t> actual = bleu.GetReferenceLengths(0);
  size_t expected[] = {5,6,6};
  BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(),actual.end(),expected, expected+3);
}

BOOST_AUTO_TEST_CASE(multi_sentence) {
  Translation ref1,ref2,src;
  TextToTranslation("fee fye fo fum",ref1);
  TextToTranslation("hee ha haw", ref2);
  TextToTranslation("ra ra ra", src);
  Bleu bleu;
  vector<Translation> refs;
  refs.push_back(ref1);
  bleu.AddReferences(refs,src);
  refs.clear();
  refs.push_back(ref2);
  bleu.AddReferences(refs,src);
  
  NGramMap ngrams = bleu.GetReferenceStats(0);
  checkNgram("fee", 1, ngrams);
  checkNgram("fye", 1, ngrams);
  checkNgram("fo", 1, ngrams);
  checkNgram("fum", 1, ngrams);
  checkNgram("fee fye", 1, ngrams);
  checkNgram("fye fo", 1, ngrams);
  checkNgram("fo fum", 1, ngrams);
  checkNgram("fee fye fo", 1, ngrams);
  checkNgram("fye fo fum", 1, ngrams);
  checkNgram("fee fye fo fum", 1, ngrams);
  BOOST_CHECK_EQUAL(ngrams.size(),(size_t)10);
  vector<size_t> actual = bleu.GetReferenceLengths(0);
  size_t expected[] = {4};
  BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(),actual.end(),expected,expected+1);
  
  ngrams = bleu.GetReferenceStats(1);
  checkNgram("hee", 1, ngrams);
  checkNgram("ha", 1, ngrams);
  checkNgram("haw", 1, ngrams);
  checkNgram("hee ha", 1, ngrams);
  checkNgram("ha haw", 1, ngrams);
  checkNgram("hee ha haw", 1, ngrams);
  BOOST_CHECK_EQUAL(ngrams.size(),(size_t)6);
  actual = bleu.GetReferenceLengths(1);
  expected[0] = 3;
  BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(),actual.end(),expected,expected+1);
}

BOOST_AUTO_TEST_CASE(source_length) {
  Translation ref, src1, src2;
  TextToTranslation("ref whatever", ref);
  TextToTranslation("the first source sentence", src1);
  TextToTranslation("the second source sentence a bit longer than the first",src2);
  vector<Translation> refs;
  refs.push_back(ref);
  Bleu bleu;
  bleu.AddReferences(refs,src1);
  bleu.AddReferences(refs,src2);
  BOOST_CHECK_EQUAL(bleu.GetSourceLength(0), (size_t)4);
  BOOST_CHECK_EQUAL(bleu.GetSourceLength(1), (size_t)10);
}

BOOST_AUTO_TEST_CASE(evaluate_1ref_1hyp_nobp) {
  Translation ref, src, hyp;
  TextToTranslation("this is the correct one ,  this one",ref);
  TextToTranslation("is this is the  one ,  this one", hyp);
  TextToTranslation("whatever",src);
  vector<Translation> refs;
  refs.push_back(ref);
  Bleu bleu;
  bleu.AddReferences(refs,src);
  vector<size_t> sentenceIds;
  sentenceIds.push_back(0);
  GainFunctionHandle gf = bleu.GetGainFunction(sentenceIds);
  float actual = gf->Evaluate(hyp);
  float sm = BLEU_SMOOTHING;
  // precisions: 7/8, 5/7, 3/6 and 1/5
  float log_expected = log(7+sm) - log(8+sm) + log(5+sm) - log(7+sm) + log(3+sm) - log(6+sm) + log(1+sm) - log(5+sm);
  log_expected /= BLEU_ORDER;
  BOOST_CHECK_CLOSE(actual,100*exp(log_expected),0.001);
}



BOOST_AUTO_TEST_CASE(evaluate_1ref_3src_nobp) {
  Translation ref0,ref1,ref2,hyp0,hyp1,hyp2,src;
  TextToTranslation("the first ref", ref0);
  TextToTranslation("the second ref 2", ref1);
  TextToTranslation("another ref",ref2);
  TextToTranslation("the first guessed ref", hyp0);
  TextToTranslation("this is the second hypothesis", hyp1);
  TextToTranslation("another ref hyp",hyp2);
  TextToTranslation("whatever",src);
  
  vector<Translation> refs(1);
  Bleu bleu;
  refs[0] = ref0;
  bleu.AddReferences(refs,src);
  refs[0] = ref1;
  bleu.AddReferences(refs,src);
  refs[0] = ref2;
  bleu.AddReferences(refs,src);
  
  vector<size_t> sentenceIds;
  sentenceIds.push_back(0);
  sentenceIds.push_back(1);
  sentenceIds.push_back(2);
  GainFunctionHandle gf = bleu.GetGainFunction(sentenceIds);
  
  vector<Translation> hyps;
  hyps.push_back(hyp0);
  hyps.push_back(hyp1);
  hyps.push_back(hyp2);
  float actual = gf->Evaluate(hyps);
  float sm = BLEU_SMOOTHING;
  //precision 7/12, 3/9, 0/6 and 0/3
  float log_expected = log(7+sm) - log(12+sm) + log(3+sm) - log(9+sm) + log(0+sm) - log(6+sm) + log(0+sm) - log(3+sm);
  log_expected /= BLEU_ORDER;
  BOOST_CHECK_EQUAL(actual, 100*exp(log_expected));
}



BOOST_AUTO_TEST_CASE(evaluate_1ref_1src_bp) {
  Translation ref, src, hyp;
  TextToTranslation("this is the correct one ,  this one",ref);
  TextToTranslation("this is the short one", hyp);
  TextToTranslation("whatever",src);
  vector<Translation> refs;
  refs.push_back(ref);
  Bleu bleu;
  bleu.AddReferences(refs,src);
  vector<size_t> sentenceIds;
  sentenceIds.push_back(0);
  GainFunctionHandle gf = bleu.GetGainFunction(sentenceIds);
  
  float actual = gf->Evaluate(hyp);
  float sm = BLEU_SMOOTHING;
  //precision: 4/5, 2/4, 1/3, 0/2
  float log_expected = log(4+sm) - log(5+sm) + log(2+sm) - log(4+sm) + log(1+sm) - log(3+sm) + log(0+sm) - log(2+sm);
  log_expected /= BLEU_ORDER;
  log_expected += (1 - 8.0/5.0);
  BOOST_CHECK_EQUAL(actual, 100*exp(log_expected));
}

BOOST_AUTO_TEST_CASE(evaluate_caching) {
  Translation ref, src, hyp1, hyp2, hyp3;
  TextToTranslation("this is the reference sentence",ref);
  TextToTranslation("this the reference sentence .", hyp1);
  TextToTranslation("the reference sentence , what ?", hyp2);
  TextToTranslation("the reference phrase . where ?", hyp3);
  TextToTranslation("whatever", src);
  Bleu bleu;
  vector<Translation> refs1;
  refs1.push_back(ref);
  bleu.AddReferences(refs1,src);
  vector<Translation> refs2;
  refs2.push_back(ref);
  bleu.AddReferences(refs2,src);
  vector<size_t> sentenceIds;
  sentenceIds.push_back(0);
  sentenceIds.push_back(1);
  GainFunctionHandle gf = bleu.GetGainFunction(sentenceIds);
  
  vector<Translation> hyps1;
  hyps1.push_back(hyp1);
  hyps1.push_back(hyp2);
  float actual = gf->Evaluate(hyps1);
  float sm = BLEU_SMOOTHING;
  //precision 7/11, 4/9, 2/7, 0/5
  float log_expected = log(7+sm) - log(11+sm) + log(4+sm) - log(9+sm) + log(2+sm) - log(7+sm) + log(0+sm) - log(5+sm);
  log_expected /= BLEU_ORDER;
  BOOST_CHECK_CLOSE(actual, 100*exp(log_expected),0.01);
  
  vector<Translation> hyps2;
  hyps2.push_back(hyp1);
  hyps2.push_back(hyp3);
  actual = gf->Evaluate(hyps2);
  //precision 6/11, 3/9, 1/7, 0/5
  log_expected = log(6+sm) - log(11+sm) + log(3+sm) - log(9+sm) + log(1+sm) - log(7+sm) + log(0+sm) - log(5+sm);
  log_expected /= BLEU_ORDER;
  BOOST_CHECK_EQUAL(actual, 100*exp(log_expected));
}


BOOST_AUTO_TEST_CASE(evaluate_3ref_1src_bp) {
  Translation ref1, ref2, ref3, src, hyp;
  TextToTranslation("what is this saying ?", ref1);
  TextToTranslation("what saying is this ? ?", ref2);
  TextToTranslation("what is this is this ?", ref3);
  TextToTranslation("is this is what", hyp);
  TextToTranslation("whatever", src);
  vector<Translation> refs;
  refs.push_back(ref1);
  refs.push_back(ref2);
  refs.push_back(ref3);
  Bleu bleu;
  bleu.AddReferences(refs,src);
  vector<size_t> sentenceIds;
  sentenceIds.push_back(0);
  GainFunctionHandle gf = bleu.GetGainFunction(sentenceIds);

  float actual = gf->Evaluate(hyp);
  float sm = BLEU_SMOOTHING;
  //precision 4/4, 2/3, 1/2, 0/1
  float log_expected = log(4+sm) - log(4+sm) + log(2+sm) - log(3+sm) + log(1+sm) - log(2+sm) + log(0+sm) - log(1+sm);
  log_expected  /= BLEU_ORDER;
  //closest
  log_expected += (1-5.0/4.0);
  BOOST_CHECK_EQUAL(actual,100*exp(log_expected));
}

BOOST_AUTO_TEST_CASE(update_smoothing) {
  Bleu bleu;
  float sw = 0.5;
  bleu.SetSmoothingWeight(sw);
  BleuStats stats = bleu.GetSmoothingStats();
  for (size_t order = 1; order < BLEU_ORDER; ++order) {
    BOOST_CHECK_CLOSE(stats.tp(order), BLEU_SMOOTHING, 0.001);
    BOOST_CHECK_CLOSE(stats.total(order), BLEU_SMOOTHING, 0.001);
  }
  
  Translation src,ref,hyp1,hyp2;
  TextToTranslation("whatever it says", src);
  TextToTranslation("this is the reference sentence ok ?", ref);
  TextToTranslation("is this  the reference sentence ok !", hyp1);
  TextToTranslation("is this  the hypothesis sentence ok !", hyp2);
  vector<Translation> refs;
  refs.push_back(ref);
  bleu.AddReferences(refs,src);
  vector<size_t> sentenceIds;
  sentenceIds.push_back(0);
  GainFunctionHandle gf = bleu.GetGainFunction(sentenceIds);
  gf->AddSmoothingStats(0,hyp1);
  gf->AddSmoothingStats(0,hyp2);
  gf->UpdateSmoothingStats();
  
  //should be the average of the two hypotheses
  stats = bleu.GetSmoothingStats();
  BOOST_CHECK_CLOSE(stats.tp(1), (5.5+BLEU_SMOOTHING)*sw, 0.001);
  BOOST_CHECK_CLOSE(stats.total(1), (7+BLEU_SMOOTHING)*sw, 0.001);
  BOOST_CHECK_CLOSE(stats.tp(2), (2+BLEU_SMOOTHING)*sw, 0.001);
  BOOST_CHECK_CLOSE(stats.total(2), (6+BLEU_SMOOTHING)*sw, 0.001);
  BOOST_CHECK_CLOSE(stats.tp(3), (1+BLEU_SMOOTHING)*sw, 0.001);
  BOOST_CHECK_CLOSE(stats.total(3), (5+BLEU_SMOOTHING)*sw, 0.001);
  BOOST_CHECK_CLOSE(stats.tp(4), (0.5+BLEU_SMOOTHING)*sw, 0.001);
  BOOST_CHECK_CLOSE(stats.total(4), (4+BLEU_SMOOTHING)*sw, 0.001);
  
  BOOST_CHECK_CLOSE(stats.src_len(), 3*sw, 0.001);
  BOOST_CHECK_CLOSE(stats.ref_len(), 7*sw, 0.001);
  BOOST_CHECK_CLOSE(stats.hyp_len(), 7*sw, 0.001);
  
  gf->AddSmoothingStats(0,hyp1);
  gf->UpdateSmoothingStats();
  
  //previous stats should get downweighted
  stats = bleu.GetSmoothingStats();
  BOOST_CHECK_CLOSE(stats.tp(1), sw*(6+sw*(5.5+BLEU_SMOOTHING)), 0.001);
  BOOST_CHECK_CLOSE(stats.total(1), sw*(7+sw*(7+BLEU_SMOOTHING)), 0.001);
  BOOST_CHECK_CLOSE(stats.tp(2), sw*(3+sw*(2+BLEU_SMOOTHING)), 0.001);
  BOOST_CHECK_CLOSE(stats.total(2), sw*(6+sw*(6+BLEU_SMOOTHING)), 0.001);
  BOOST_CHECK_CLOSE(stats.tp(3), sw*(2+sw*(1+BLEU_SMOOTHING)), 0.001);
  BOOST_CHECK_CLOSE(stats.total(3), sw*(5+sw*(5+BLEU_SMOOTHING)), 0.001);
  BOOST_CHECK_CLOSE(stats.tp(4), sw*(1+sw*(0.5+BLEU_SMOOTHING)), 0.001);
  BOOST_CHECK_CLOSE(stats.total(4), sw*(4+sw*(4+BLEU_SMOOTHING)), 0.001);
  
  BOOST_CHECK_CLOSE(stats.src_len(), sw*(3+sw*(3)), 0.001);
  BOOST_CHECK_CLOSE(stats.ref_len(), sw*(7+sw*(7)), 0.001);
  BOOST_CHECK_CLOSE(stats.hyp_len(), sw*(7+sw*(7)), 0.001);
  
  
}

BOOST_AUTO_TEST_CASE(evaluate_smoothing) {
  Bleu bleu;
  float sw = 0.5;
  bleu.SetSmoothingWeight(sw);
  Translation src,ref,hyp1,hyp2;
  TextToTranslation("whatever it says", src);
  TextToTranslation("this is the reference sentence ok ?", ref);
  TextToTranslation("is this  the reference sentence ok !", hyp1);
  TextToTranslation("is this  the hypothesis sentence ok !", hyp2);
  vector<Translation> refs;
  refs.push_back(ref);
  bleu.AddReferences(refs,src);
  vector<size_t> sentenceIds;
  sentenceIds.push_back(0);
  GainFunctionHandle gf = bleu.GetGainFunction(sentenceIds);
  
  float actual = gf->Evaluate(hyp1);
  //should just be standard smoothed bleu
  float sm = BLEU_SMOOTHING;
  //precision 6/7, 3/6, 2/5, 1/4
  float log_expected = log(6+sm) - log(7+sm) + log(3+sm) - log(6+sm) + log(2+sm) - log(5+sm) + log(1+sm) - log(4+sm);
  log_expected  /= BLEU_ORDER;
  log_expected += log(3); //multiply by source length
  BOOST_CHECK_CLOSE(actual,exp(log_expected),0.001);
  
  gf->AddSmoothingStats(0,hyp1);
  gf->UpdateSmoothingStats();
  
  //now should get smoothed bleu
  actual = gf->Evaluate(hyp2);
  //precision 5/7 1/6 0/5 0/4
  log_expected = log(5+sw*(6+sm)) - log(7+sw*(7+sm)) + log(1+sw*(3+sm)) - log(6+sw*(6+sm)) + log(0+sw*(2+sm)) 
      - log(5 + sw*(5+sm)) + log(0 + sw*(1+sm)) - log(4+sw*(4+sm));
  //cerr << log(5+sw*(6+sm)) << " " << log(7+sw*(7+sm)) << " " <<  log(1+sw*(3+sm))  << " " <<
  //    log(6+sw*(6+sm)) << " " << log(0+sw*(2+sm)) << " " << log(5 + sw*(5+sm)) << " " << 
  //    log(0 + sw*(1+sm)) << " " << log(4+sw*(4+sm)) << endl;
  log_expected /= BLEU_ORDER;
  log_expected += log(3 + sw*3); // weight bleu by (O_f + |f|)
  BOOST_CHECK_CLOSE(actual,exp(log_expected),0.001);
  
}

BOOST_AUTO_TEST_SUITE_END()


