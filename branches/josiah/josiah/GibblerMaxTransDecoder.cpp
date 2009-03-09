#include "GibblerMaxTransDecoder.h"

#include <sstream>
#include <map>

using namespace __gnu_cxx;
using namespace std;

namespace Moses {

  GibblerMaxTransDecoder::GibblerMaxTransDecoder() : n(0), m_outputMaxChange(false) {}



string ToString(const vector<const Factor*>& ws) {
  ostringstream os;
  for (vector<const Factor*>::const_iterator i = ws.begin(); i != ws.end(); ++i)
    os << (*i)->GetString() << " ";
  return os.str();
}

void GibblerMaxTransDecoder::collect(Sample& sample) {
  ++n;
  const Hypothesis* h = sample.GetSampleHypothesis();
  vector<const Factor*> trans;
  h->GetTranslation(&trans, 0);
  ++samples[trans];
  
  if (m_outputMaxChange) {
    vector<const Factor*> newmax = Max();
    if (newmax != m_maxTranslation) {
      m_maxTranslation = newmax;
      cerr << "NewMaxTrans(" << n << ") ";
      cerr << ToString(m_maxTranslation);
      cerr << endl;
    }
  }
}

vector<const Factor*> GibblerMaxTransDecoder::Max() {
  hash_map<vector<const Factor*>, int>::const_iterator ci;
  multimap<float, const vector<const Factor*>*,greater<float> > sorted;
  const float nf = n;
  for (ci = samples.begin(); ci != samples.end(); ++ci) {
    sorted.insert(make_pair<float, const vector<const Factor*>*>(static_cast<float>(ci->second) / nf, &ci->first));
  }
  multimap<float, const vector<const Factor*>*>::iterator i;
  for (i = sorted.begin(); i != sorted.end(); ++i)
    VERBOSE(1, i->first << "\t" << ToString(*i->second) << endl);
  return *sorted.begin()->second;
}

}
