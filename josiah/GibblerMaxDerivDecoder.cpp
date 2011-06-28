#include "GibblerMaxDerivDecoder.h"
#include "StaticData.h"
#include "MpiDebug.h"
#include <iomanip>


using namespace Moses;
using namespace std;

namespace Josiah {

  void DerivationCollector::outputDerivationProbability(const DerivationProbability& dp,size_t n,  std::ostream& out) {
  out << std::setprecision(8) << dp.second << " " << dp.second*n <<" " << *(dp.first);
}
  
  void DerivationCollector::reset() {
    MaxCollector<Derivation>::reset();
    m_derivByTrans.clear();
  }
  
  void DerivationCollector::collect(Sample& sample) {
    collectSample(Derivation(sample)); 
    IFVERBOSE(1) {
      VERBOSE(1,"Collected: " << Derivation(sample) << endl);
    }
    if (m_collectDerivByTrans) {
      //derivations per translation
      Derivation d(sample);
      ostringstream os;
      vector<string> sentence;
      d.getTargetSentence(sentence);
      copy(sentence.begin(),sentence.end(),ostream_iterator<string>(os," "));
      m_derivByTrans[os.str()].insert(d);
    }
    size_t n = N() + 1;
    if (m_pd > 0 && n > 0 && n%m_pd == 0) {
      pair<const Derivation*,float> max = getMax();
      if (max.first) {
        MPI_VERBOSE(0, "MaxDeriv(" << n << "): " << std::setprecision(8) << max.second << " " << max.second*n <<" " << *(max.first) << endl) 
        MPI_VERBOSE(0, "DerivEntropy(" << n << "): " << getEntropy() << endl)
      }
    }
  }
  
  
  
  
  
  void DerivationCollector::outputDerivationsByTranslation(ostream& out) {
    out << "Derivations per translation" << endl;
    multimap<size_t,string,greater<size_t> > sortedCounts;
    for (map<string, set<Derivation> >::const_iterator i = m_derivByTrans.begin();
         i != m_derivByTrans.end(); ++i) {
           sortedCounts.insert(pair<size_t,string>(i->second.size(),i->first));
         }
    
         for (multimap<size_t,string, greater<size_t> >::const_iterator i = sortedCounts.begin(); i != sortedCounts.end(); ++i) {
           out << "COUNT: " <<  i->first << " TRANS:" << i->second << endl;
           if (i->first > 1) {
             for (set<Derivation>::const_iterator j = m_derivByTrans[i->second].begin(); 
                  j != m_derivByTrans[i->second].end(); ++j) {
                    out << *j << endl;
                  }
           }
         }
    
  }

 
  /**argmax and max*/
  std::pair<const Derivation*,float> DerivationCollector::getMAP() const {
    const Derivation* argmax = NULL;
    float max = -10000;
    map<const Derivation*,double> p;
    getDistribution(p);
    for (map<const Derivation*,double>::const_iterator pi = p.begin(); pi != p.end(); ++pi) {
      float score = pi->first->getScore();
      if (score > max) {
        max = score;
        argmax = pi->first;
      }
    }
    
    return pair<const Derivation*,float>(argmax,max);
  }

}
