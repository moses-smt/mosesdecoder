#include "SentenceBleu.h"

#include <valarray>
#include <vector>

#include "Phrase.h"
#include "GibblerExpectedLossTraining.h"

using namespace std;

namespace Josiah {

SentenceBLEU::SentenceBLEU(int n, const std::vector<std::string>& refs) :
 n_(n) {
  for (vector<string>::const_iterator ci = refs.begin();
     ci != refs.end(); ++ci) {
    vector<const Factor*> fv;
    GainFunction::ConvertStringToFactorArray(*ci, &fv);
    lengths_.push_back(fv.size());
    CountRef(fv, ngrams_);
  }
}

SentenceBLEU::SentenceBLEU(int n, const vector<const Factor*> & ref) :
  n_(n) {
  lengths_.push_back(ref.size());
  CountRef(ref, ngrams_);
}  
  
  
void SentenceBLEU::CountRef(const vector<const Factor*>& ref, NGramCountMap& ngrams) const{
  NGramCountMap tc;

  vector<const Factor*> ngram(n_);
  int s = ref.size();
  for (int j=0; j<s; ++j) {
    int remaining = s-j;
    int k = (n_ < remaining ? n_ : remaining);
    ngram.clear();
    for (int i=1; i<=k; ++i) {
      ngram.push_back(ref[j + i - 1]);
      tc[ngram].first++;
    }
  }

  // clipping
  for (NGramCountMap::iterator i = tc.begin(); i != tc.end(); ++i) {
    pair<int,int>& p = ngrams[i->first];
    if (p.first < i->second.first)
      p = i->second;
  }
}

float SentenceBLEU::ComputeGain(const vector<const Factor*>& sent) const {
  NGramCountMap sentNgrams;
  CountRef(sent, sentNgrams);
  
  return CalcScore(ngrams_, sentNgrams, sent.size());
}
  
float SentenceBLEU::CalcScore(const NGramCountMap & refNgrams, const NGramCountMap & hypNgrams, int hypLen) const {
  
  valarray<int> hyp(n_);
  valarray<int> correct(n_);
 
  float ref_len = GetClosestLength(hypLen);
  
  for (NGramCountMap::const_iterator hypIt = hypNgrams.begin();
       hypIt != hypNgrams.end(); ++hypIt)
  {
    NGramCountMap::iterator refIt = ngrams_.find(hypIt->first);
    
    if(refIt != refNgrams.end())
    {
      correct[hypIt->first.size() - 1] += min(refIt->second.first, hypIt->second.first); 
    }
    
    hyp[hypIt->first.size() - 1] += hypIt->second.first;
  }
  
  return CalcBleu(hyp, correct, ref_len, (float) hypLen);
}
  
  
float SentenceBLEU::ComputeGain(const GainFunction& hyp) const {
  assert(hyp.GetType() == this->GetType());
  const NGramCountMap& hyp_ngrams = static_cast<const SentenceBLEU&>(hyp).GetNgrams();

  return CalcScore(ngrams_, hyp_ngrams, static_cast<const SentenceBLEU&>(hyp).GetLength());
}  

float SentenceBLEU::CalcBleu(const valarray<int> & hyp, const valarray<int> & correct, float ref_len, float hyp_len) const{
    
  float log_bleu = 0;
  int count = 0;
  for (int i = 0; i < n_; ++i) {
    if (true || hyp[i] > 0) {
      float lprec = log(0.01 + correct[i]) - log(0.01 + hyp[i]);
      log_bleu += lprec;
      ++count;
    }
  }
  log_bleu /= static_cast<float>(count);
  float lbp = 0.0;
  if (hyp_len < ref_len)
    lbp = (hyp_len - ref_len) / hyp_len;
  log_bleu += lbp;
  return exp(log_bleu);
}  
  
}

