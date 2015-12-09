#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include "moses/TrellisPathList.h"
#include "moses/TrellisPath.h"
// #include "moses/StaticData.h"
#include "moses/Util.h"
#include "mbr.h"

using namespace std ;
using namespace Moses;


/* Input :
   1. a sorted  n-best list, with duplicates filtered out in the following  format
   0 ||| amr moussa is currently on a visit to libya , tomorrow , sunday , to hold talks with regard to the in sudan . ||| 0 -4.94418 0 0 -2.16036 0 0 -81.4462 -106.593 -114.43 -105.55 -12.7873 -26.9057 -25.3715 -52.9336 7.99917 -24 ||| -4.58432

   2. a weight vector
   3. bleu order ( default = 4)
   4. scaling factor to weigh the weight vector (default = 1.0)

   Output :
   translations that minimise the Bayes Risk of the n-best list


*/

int BLEU_ORDER = 4;
int SMOOTH = 1;
float min_interval = 1e-4;
void extract_ngrams(const vector<const Factor* >& sentence, map < vector < const Factor* >, int >  & allngrams)
{
  vector< const Factor* > ngram;
  for (int k = 0; k < BLEU_ORDER; k++) {
    for(int i =0; i < max((int)sentence.size()-k,0); i++) {
      for ( int j = i; j<= i+k; j++) {
        ngram.push_back(sentence[j]);
      }
      ++allngrams[ngram];
      ngram.clear();
    }
  }
}

float calculate_score(const vector< vector<const Factor*> > & sents, int ref, int hyp,  vector < map < vector < const Factor *>, int > > & ngram_stats )
{
  int comps_n = 2*BLEU_ORDER+1;
  vector<int> comps(comps_n);
  float logbleu = 0.0, brevity;

  int hyp_length = sents[hyp].size();

  for (int i =0; i<BLEU_ORDER; i++) {
    comps[2*i] = 0;
    comps[2*i+1] = max(hyp_length-i,0);
  }

  map< vector < const Factor * > ,int > & hyp_ngrams = ngram_stats[hyp] ;
  map< vector < const Factor * >, int > & ref_ngrams = ngram_stats[ref] ;

  for (map< vector< const Factor * >, int >::iterator it = hyp_ngrams.begin();
       it != hyp_ngrams.end(); it++) {
    map< vector< const Factor * >, int >::iterator ref_it = ref_ngrams.find(it->first);
    if(ref_it != ref_ngrams.end()) {
      comps[2* (it->first.size()-1)] += min(ref_it->second,it->second);
    }
  }
  comps[comps_n-1] = sents[ref].size();

  for (int i=0; i<BLEU_ORDER; i++) {
    if (comps[0] == 0)
      return 0.0;
    if ( i > 0 )
      logbleu += log((float)comps[2*i]+SMOOTH)-log((float)comps[2*i+1]+SMOOTH);
    else
      logbleu += log((float)comps[2*i])-log((float)comps[2*i+1]);
  }
  logbleu /= BLEU_ORDER;
  brevity = 1.0-(float)comps[comps_n-1]/comps[1]; // comps[comps_n-1] is the ref length, comps[1] is the test length
  if (brevity < 0.0)
    logbleu += brevity;
  return exp(logbleu);
}

const TrellisPath doMBR(const TrellisPathList& nBestList, AllOptions const& opts)
{
  float marginal = 0;
  float mbr_scale = opts.mbr.scale;
  vector<float> joint_prob_vec;
  vector< vector<const Factor*> > translations;
  float joint_prob;
  vector< map < vector <const Factor *>, int > > ngram_stats;

  TrellisPathList::const_iterator iter;

  // get max score to prevent underflow
  float maxScore = -1e20;
  for (iter = nBestList.begin() ; iter != nBestList.end() ; ++iter) {
    const TrellisPath &path = **iter;
    float score = mbr_scale * path.GetScoreBreakdown()->GetWeightedScore();
    if (maxScore < score) maxScore = score;
  }

  vector<FactorType> const& oFactors = opts.output.factor_order;
  UTIL_THROW_IF2(oFactors.size() != 1, "Need exactly one output factor!");
  for (iter = nBestList.begin() ; iter != nBestList.end() ; ++iter) {
    const TrellisPath &path = **iter;
    joint_prob = UntransformScore(mbr_scale * path.GetScoreBreakdown()->GetWeightedScore() - maxScore);
    marginal += joint_prob;
    joint_prob_vec.push_back(joint_prob);

    // get words in translation
    vector<const Factor*> translation;
    GetOutputFactors(path, oFactors[0], translation);

    // collect n-gram counts
    map < vector < const Factor *>, int > counts;
    extract_ngrams(translation,counts);

    ngram_stats.push_back(counts);
    translations.push_back(translation);
  }

  vector<float> mbr_loss;
  float bleu, weightedLoss;
  float weightedLossCumul = 0;
  float minMBRLoss = 1000000;
  int minMBRLossIdx = -1;

  /* Main MBR computation done here */
  iter = nBestList.begin();
  for (unsigned int i = 0; i < nBestList.GetSize(); i++) {
    weightedLossCumul = 0;
    for (unsigned int j = 0; j < nBestList.GetSize(); j++) {
      if ( i != j) {
        bleu = calculate_score(translations, j, i,ngram_stats );
        weightedLoss = ( 1 - bleu) * ( joint_prob_vec[j]/marginal);
        weightedLossCumul += weightedLoss;
        if (weightedLossCumul > minMBRLoss)
          break;
      }
    }
    if (weightedLossCumul < minMBRLoss) {
      minMBRLoss = weightedLossCumul;
      minMBRLossIdx = i;
    }
    iter++;
  }
  /* Find sentence that minimises Bayes Risk under 1- BLEU loss */
  return nBestList.at(minMBRLossIdx);
  //return translations[minMBRLossIdx];
}

void
GetOutputFactors(const TrellisPath &path, FactorType const oFactor,
                 vector <const Factor*> &translation)
{
  const std::vector<const Hypothesis *> &edges = path.GetEdges();

  // print the surface factor of the translation
  for (int currEdge = (int)edges.size() - 1 ; currEdge >= 0 ; currEdge--) {
    const Hypothesis &edge = *edges[currEdge];
    const Phrase &phrase = edge.GetCurrTargetPhrase();
    size_t size = phrase.GetSize();
    for (size_t pos = 0 ; pos < size ; pos++) {
      const Factor *factor = phrase.GetFactor(pos, oFactor);
      translation.push_back(factor);
    }
  }
}

