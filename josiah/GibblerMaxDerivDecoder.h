#pragma once

#include <functional>
#include <string>
#include <vector>
#include <set>

#include "Derivation.h"
#include "GibblerMaxTransDecoder.h"

namespace Josiah {

  class DerivationCollector: public virtual MaxCollector<Derivation> {
    public:
      DerivationCollector(): MaxCollector<Derivation>("Deriv"),  m_pd(0) ,m_collectDerivByTrans(false) {}
      void collect(Sample& sample);
      /** Write max periodically to stderr */
      void setPeriodicDecode(int pd) {m_pd = pd;}
      void setCollectDerivationsByTranslation(bool dbyt) {m_collectDerivByTrans = dbyt;}
      void outputDerivationsByTranslation(std::ostream& out);
      void outputDerivationProbability(const DerivationProbability& dp,size_t n, std::ostream& out);
      void reset();  
      virtual ~DerivationCollector(){}
      std::pair<const Derivation*,float> getMAP() const;
    private:
      std::map<std::string,std::set<Derivation> > m_derivByTrans;
      int m_pd;
      bool m_collectDerivByTrans;
  };
}
