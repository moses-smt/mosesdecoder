
#pragma once

#include <string>
#include <vector>

#include "Derivation.h"
#include "GibblerMaxTransDecoder.h"

namespace Josiah {

  class DerivationCollector: public virtual MaxCollector<Derivation> {
    public:
      DerivationCollector(): m_n(0),m_pd(0) ,m_collectDerivByTrans(false), m_outputMaxChange(false), m_maxDerivation(NULL) {}
      void collect(Moses::Sample& sample);
      /** Top n in descending order. */
      void getTopN(size_t n, std::vector<DerivationProbability>& derivations);
      virtual void Max(std::vector<const Moses::Factor*>& translation, size_t& count);
      /** Write max periodically to stderr */
      void setPeriodicDecode(int pd) {m_pd = pd;}
      /** Write the max derivation every time it changes */
      void setOutputMaxChange(bool outputMaxChange) {m_outputMaxChange = outputMaxChange;}
      virtual void getCounts(vector<size_t>& counts) const;
      void setCollectDerivationsByTranslation(bool dbyt) {m_collectDerivByTrans = dbyt;}
      void outputDerivationsByTranslation(std::ostream& out);
      void outputDerivationProbability(const DerivationProbability& dp, std::ostream& out);
        
    private:
      std::map<Derivation,size_t> m_counts;
      std::map<std::string,std::set<Derivation> > m_derivByTrans;
      size_t m_n;
      int m_pd;
      bool m_collectDerivByTrans;
      bool m_outputMaxChange;
      const Derivation* m_maxDerivation;
  };

}
