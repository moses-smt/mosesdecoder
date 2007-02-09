//
// C++ Implementation: bleuloss
//
// Description:
//
//
// Author: Abhishek Arun <s0343799@alton.inf.ed.ac.uk>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "BleuLoss.h"
#include "Phrase.h"

using namespace std;

#define PERFECT_BLEU 1
#define FIRST_HYP 0

//Changed passing by value to passing by reference - AA 4/8/06 10 55
void BleuLoss::extract_ngrams(const Phrase& sentence, map< vector< WORD_ID >, int > &counts)
{
  vector< WORD_ID> ngram;
  for (int k = 0; k< ngram_size; k++)
  {
    for(int i =0; i < max((int)sentence.GetSize()-k,0); i++)
    {
      for ( int j = i; j<= i+k; j++)
      {
				ngram.push_back(sentence.GetFactor(j, 0)); ///  TODO get factor id
      }
      ++counts[ngram];
      ngram.clear();
    }
  }
}


double BleuLoss::calculate_score(const Phrase &hyp, const Phrase &ref, map < vector < WORD_ID >, int > & ref_ngrams){

   map < vector < WORD_ID >, int > hyp_ngrams; 
   extract_ngrams(hyp,hyp_ngrams);

   const int comps_n = 2*ngram_size+1;
   vector<size_t> comps(comps_n);
   double logbleu = 0.0, brevity;
   
   size_t hyp_length = hyp.GetSize();
   size_t ref_length = ref.GetSize();
 
   for (int i =0; i<ngram_size;i++)
   {
     comps[2*i] = 0;
     comps[2*i+1] = max(hyp_length-i, (size_t) 0);
   }

   for (map< vector< WORD_ID >, int >::iterator it = hyp_ngrams.begin();
       it != hyp_ngrams.end(); it++)
  {
    map< vector< WORD_ID >, int >::iterator ref_it = ref_ngrams.find(it->first);
    if(ref_it != ref_ngrams.end())
    {
      comps[2* (it->first.size()-1)] += min(ref_it->second,it->second);
    }
  }   
 
  comps[comps_n-1] = ref_length;

   /*if (pharaoh->verbose >= 4)
  {
    for ( int i = 0; i < comps_n; i++)
      cerr << "Comp " << i << " : " << comps[i];
  }*/

  for (int i=0; i<ngram_size; i++)
  {
    if (comps[0] == 0)
      return 0.0;
    if ( i > 0)
      logbleu += (double)log((double)comps[2*i]+smoothing_constant)-(double)log((double)comps[2*i+1]+smoothing_constant);
    else
      logbleu += (double)log((double)comps[2*i])-(double)log((double)comps[2*i+1]);
  }
  logbleu /= ngram_size;
  brevity = 1.0-(double)comps[comps_n-1]/comps[1]; // comps[comps_n-1] is the ref length, comps[1] is the test length
  if (brevity < 0.0)
    logbleu += brevity;
  //cerr << "Score : " << exp(logbleu);
  return exp(logbleu);
}


double BleuLoss::getScore(const Phrase &reference, const Phrase &hypothesis)
{
  double score = 0.0;
  extract_ngrams(reference, ref_counts);
  score = calculate_score(hypothesis, reference, ref_counts);
  //if(pharaoh->verbose >= 2){
  //   cerr << "Smoothed BLEU is " << score << endl;
  //}
  cerr << reference << endl << hypothesis << endl << score << endl;

  return score;
}
   

