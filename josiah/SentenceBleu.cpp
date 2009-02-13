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
    CountRef(fv);
  }
}

void SentenceBLEU::CountRef(const vector<const Factor*>& ref) {
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
    pair<int,int>& p = ngrams_[i->first];
    if (p.first < i->second.first)
      p = i->second;
  }
}

float SentenceBLEU::ComputeGain(const vector<const Factor*>& sent) const {
  for (NGramCountMap::iterator i = ngrams_.begin(); i != ngrams_.end(); ++i)
    i->second.second = 0;
  vector<const Factor*> ngram(n_);
  valarray<int> hyp(n_);
  valarray<int> correct(n_);
  int s = sent.size();
  for (int j=0; j<s; ++j) {
    int remaining = s-j;
    int k = (n_ < remaining ? n_ : remaining);
    ngram.clear();
    for (int i=1; i<=k; ++i) {
        ngram.push_back(sent[j + i - 1]);
        pair<int,int>& p = ngrams_[ngram];
        if (p.second < p.first) {
          ++p.second;
          correct[i-1]++;
        }
        // if the 1 gram isn't found, don't try to match don't need to match any 2- 3- .. grams:
        if (!p.first) {
          for (; i<=k; ++i)
            hyp[i-1]++;
        } else {
          hyp[i-1]++;
        }
      }
  }
  const float ref_len = GetClosestLength(sent.size());
  const float hyp_len = sent.size();

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

