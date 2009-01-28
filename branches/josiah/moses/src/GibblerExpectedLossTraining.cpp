#include "GibblerExpectedLossTraining.h"

using namespace std;

namespace Moses {

#if 0
class BLEUScorerBase {
 public:
  BLEUScorerBase(const std::vector<Phrase>& references,
             bool case_sensitive,
             int n
             );
  Score* ScoreCandidate(const Phrase& hyp) const;

 protected:
  virtual float ComputeRefLength(const vector<WordID>& hyp) const = 0;
 private:
  struct NGramCompare {
    int operator() (const vector<WordID>& a, const vector<WordID>& b) {
      size_t as = a.size();
      size_t bs = b.size();
      const size_t s = (as < bs ? as : bs);
      for (size_t i = 0; i < s; ++i) {
         int d = a[i] - b[i];
         if (d < 0) return true;
	 if (d > 0) return false;
      }
      return as < bs;
    }
  };
  typedef map<vector<WordID>, pair<int,int>, NGramCompare> NGramCountMap;
  void CountRef(const vector<WordID>& ref) {
    NGramCountMap tc;
    vector<WordID> ngram(n_);
    int s = ref.size();
    for (int j=0; j<s; ++j) {
      int remaining = s-j;
      int k = (n_ < remaining ? n_ : remaining);
      ngram.clear();
      for (int i=1; i<=k; ++i) {
        int l = s-i;
        int c = 0;
	ngram.push_back(ref[j + i - 1]);
        tc[ngram].first++;
      }
    }
    for (NGramCountMap::iterator i = tc.begin(); i != tc.end(); ++i) {
      pair<int,int>& p = ngrams_[i->first];
      if (p.first < i->second.first)
        p = i->second;
    }
  }

  void ComputeNgramStats(const vector<WordID>& sent,
       valarray<int>* correct,
GibblerExpectedLossCollector

#endif

GibblerExpectedLossCollector::GibblerExpectedLossCollector() :
  g(NULL), sent_num(0) {}

void GibblerExpectedLossCollector::collect(Sample& s) {
  ++n;  // increment total samples seen
  const Hypothesis* h = s.GetSampleHypothesis();
  vector<const Factor*> trans;
  h->GetTranslation(&trans, 0);
  const float gain = g->ComputeGain(trans, refs[sent_num]);
  samples.push_back(make_pair(s.GetFeatureValues(), gain));
  feature_expectations.PlusEquals(s.GetFeatureValues());
}

ScoreComponentCollection GibblerExpectedLossCollector::ComputeGradient() {
  feature_expectations.DivideEquals(n);
  ScoreComponentCollection grad = feature_expectations; grad.ZeroAll();
  list<pair<ScoreComponentCollection, float> >::iterator si;
  for (si = samples.begin(); si != samples.end(); ++si) {
    ScoreComponentCollection d = si->first;
    const float gain = si->second;
    d.MinusEquals(feature_expectations);
    d.MultiplyEquals(gain);
    grad.PlusEquals(d);
  }
  grad.DivideEquals(n);

  gradient.PlusEquals(grad);
}

}

