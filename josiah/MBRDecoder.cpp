#include "MBRDecoder.h"

#include <sstream>
#include <map>
#include "SentenceBleu.h"

using namespace __gnu_cxx;
using namespace std;

namespace Josiah {

void MBRDecoder::collect(Sample& sample) {
  ++n;
  const Hypothesis* h = sample.GetSampleHypothesis();
  vector<const Factor*> trans;
  h->GetTranslation(&trans, 0);
  ++samples[trans];
}
  
string ToString(const vector<const Factor*>& ws) {
    ostringstream os;
    for (vector<const Factor*>::const_iterator i = ws.begin(); i != ws.end(); ++i)
      os << (*i)->GetString() << " ";
    return os.str();
}
  

vector<const Factor*> MBRDecoder::Max() {
  //sort the translations by their probability
  hash_map<vector<const Factor*>, int>::const_iterator ci;
  multimap<float, const vector<const Factor*>*,greater<float> > sorted;
  const float nf = n;
  for (ci = samples.begin(); ci != samples.end(); ++ci) {
    sorted.insert(make_pair<float, const vector<const Factor*>*>(static_cast<float>(ci->second) / nf, &ci->first));
  }
  multimap<float, const vector<const Factor*>*>::iterator i;
  for (i = sorted.begin(); i != sorted.end(); ++i)
    VERBOSE(1, i->first << "\t" << ToString(*i->second) << endl);
  
  //Posterior probs computed using the whole evidence set
  //MBR decoding using configurable size
  int topN = 0;
  vector<pair<const vector<const Factor*>*, float> > topNTranslations;
  
  for (i = sorted.begin(); i != sorted.end(); ++i) {
    topNTranslations.push_back(make_pair(i->second, i->first));
    //Calc the sufficient statistics for the translation
    g.push_back(new SentenceBLEU(4,*i->second));
    if (topN == mbrSize)
      break;
    ++topN;
  }
  
  float bleu(0.0), weightedLoss(0.0), weightedLossCumul(0.0), minMBRLoss(100000);
  vector<float> mbrLoss;
  int minMBRLossIdx(-1);
  
  //Main MBR computation done here
  for(size_t i = 0; i < topNTranslations.size(); ++i) {
    weightedLossCumul = 0.0;
    const GainFunction& gf = g[i];
    for(size_t j = 0; j < topNTranslations.size(); ++j) {
      if (i != j) {
        bleu = gf.ComputeGain(g[j]);
        weightedLoss = (1- bleu) * topNTranslations[j].second;
        weightedLossCumul += weightedLoss;
        if (weightedLossCumul > minMBRLoss)
          break;
      }
    }  
    if (weightedLossCumul < minMBRLoss){
      minMBRLoss = weightedLossCumul;
      minMBRLossIdx = i;
    }
  }
  
  return *(topNTranslations[minMBRLossIdx].first);
}

}
