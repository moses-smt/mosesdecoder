#include "GibblerMaxDerivDecoder.h"

using namespace Moses;
using namespace std;

namespace Josiah {

void DerivationCollector::outputDerivationProbability(const DerivationProbability& dp, std::ostream& out) {
  out << std::setprecision(8) << dp.second << " " << dp.second*m_n <<" " << *(dp.first);
}
  
  
  
  void DerivationCollector::collect(Sample& sample) {
    ++m_counts[Derivation(sample)];
    collectSample(Derivation(sample)); //FIXME This should be the only collection
    IFVERBOSE(1) {
      VERBOSE(1,"Collected: " << Derivation(sample) << endl);
    }
    if (m_collectDerivByTrans) {
      //derivations per translation
      Derivation d(sample);
      ostringstream os;
      const vector<string>& sentence = d.getTargetSentence();
      copy(sentence.begin(),sentence.end(),ostream_iterator<string>(os," "));
      m_derivByTrans[os.str()].insert(d);
    }
    ++m_n;
    if (m_pd > 0 && m_n > 0 && m_n%m_pd == 0) {
      vector<DerivationProbability> derivations;
      getTopN(1,derivations);
      if (derivations.size()) {
        cerr << "MaxDeriv(" << m_n << "): ";
        outputDerivationProbability(derivations[0],cerr);
        cerr << endl;
        cerr << "DerivEntropy(" << m_n << "): " << getEntropy() << endl;
      }
    }
    
    if (m_outputMaxChange) {
      vector<DerivationProbability> derivations;
      getTopN(1,derivations);
      const Derivation* newmax = derivations[0].first;
      if (m_maxDerivation == NULL || *m_maxDerivation < *newmax || *newmax < *m_maxDerivation) {
        cerr << "NewMaxDeriv(" << m_n << ")";
        outputDerivationProbability(derivations[0],cerr);
        cerr << endl;
        m_maxDerivation = newmax;
      } 
    }
    
  }
  
  void DerivationCollector::getTopN(size_t n, vector<DerivationProbability>& derivations) {
    derivations.clear();
    for (map<Derivation,size_t>::iterator i = m_counts.begin(); i != m_counts.end(); ++i) {
      float probability = (float)i->second/m_n;
      derivations.push_back(DerivationProbability(&(i->first),probability));
    }
    DerivationProbGreaterThan comparator;
    /*for (size_t i = 0; i < derivations.size(); ++i) {
    const Derivation* d = derivations[i].first;
    float probability = derivations[i].second;
    cout << *d << endl;
    cout << probability << endl;
  }*/
    sort(derivations.begin(),derivations.end(),comparator);
    while (derivations.size() > n) {
      derivations.pop_back();
    }
    //cout << derivations.size() << endl;
  }
  
  void DerivationCollector::Max(std::vector<const Factor*>& translation, size_t& count) {
    vector<DerivationProbability> derivations;
    getTopN(1,derivations);
    count = m_counts[*(derivations[0].first)];
    derivations[0].first->getTargetFactors(translation);
  }
  
  void DerivationCollector::outputDerivationsByTranslation(ostream& out) {
    out << "Derivations per translation" << endl;
    multimap<size_t,string,greater<size_t> > sortedCounts;
    for (map<string, set<Derivation> >::const_iterator i = m_derivByTrans.begin();
         i != m_derivByTrans.end(); ++i) {
           sortedCounts.insert(pair<size_t,string>(i->second.size(),i->first));
         }
    
         for (multimap<size_t,string>::const_iterator i = sortedCounts.begin(); i != sortedCounts.end(); ++i) {
           out << "COUNT: " <<  i->first << " TRANS:" << i->second << endl;
           if (i->first > 1) {
             for (set<Derivation>::const_iterator j = m_derivByTrans[i->second].begin(); 
                  j != m_derivByTrans[i->second].end(); ++j) {
                    out << *j << endl;
                  }
           }
         }
    
  }
  

  void Josiah::DerivationCollector::getCounts( vector< size_t > & counts ) const
  {
    for (map<Derivation,size_t>::const_iterator i = m_counts.begin(); i != m_counts.end(); ++i) {
      counts.push_back(i->second);
    }
  }
  
  
}
