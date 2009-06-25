#include "CorpusSamplerAnnealed.h"

#include "GainFunction.h"
#include "Hypothesis.h"
#include "Derivation.h"
#ifdef MPI_ENABLED
#include <mpi.h>
#endif

using namespace std;

namespace Josiah {
  
  ScoreComponentCollection CorpusSamplerAnnealedCollector::getExpectedFeatureValue(std::map<const Derivation*,double>& m_p) {
    ScoreComponentCollection expFV;
    for (std::map<const Derivation*,double>::const_iterator it = m_p.begin(); it != m_p.end(); ++it) {
     const Derivation* deriv = it->first;
      ScoreComponentCollection fv = deriv->getFeatureValues();  
      fv.MultiplyEquals(it->second);
      expFV.PlusEquals(fv);
    }
    return expFV;
  }
  
  void CorpusSamplerAnnealedCollector::setRegularisationGradientFactor(std::map<const Derivation*,double>& m_p) {
    double temperature = GetTemperature();
    ScoreComponentCollection expFV = getExpectedFeatureValue(m_p);
    float entropy_factor;
    for (std::map<const Derivation*,double>::const_iterator it = m_p.begin(); it != m_p.end(); ++it) {
      entropy_factor = -temperature * it->second * (log (it->second)+1);
      ScoreComponentCollection fv = it->first->getFeatureValues();  
      fv.MinusEquals(expFV);
      fv.MultiplyEquals(entropy_factor);
      m_gradient.PlusEquals(fv);
    }
  }
  
  void CorpusSamplerAnnealedCollector:: setRegularisation(std::map<const Derivation*,double>& m_p) {
    float entropy(0.0);
    for (std::map<const Derivation*,double>::const_iterator it = m_p.begin(); it != m_p.end(); ++it) {
      entropy  -= it->second*log(it->second);
    }
    m_regularisation += GetTemperature() * entropy;
  }

#ifdef MPI_ENABLED
  void CorpusSamplerAnnealedCollector::AggregateSamples(int rank) {
    AggregateSuffStats(rank);
    AggregateRegularisationStats(rank);
  }
  
  void CorpusSamplerAnnealedCollector::AggregateRegularisationStats(int rank) {  
    ScoreComponentCollection regularizationGrad;
    float regularizationFactor;
    vector<float> recvRegGrad(regularizationGrad.size());
    
    //Reduce regularization
    float reg = getRegularisation();
    if (MPI_SUCCESS != MPI_Reduce(&reg, &regularizationFactor, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    
    //Reduce regularization gradient
    vector<float> regGrad = getRegularisationGradientFactor().data();
    if (MPI_SUCCESS != MPI_Reduce(const_cast<float*>(&regGrad[0]), &recvRegGrad[0], regGrad.size(), MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    
    if (rank == 0 ) {  
      m_gradient = ScoreComponentCollection(recvRegGrad);
      m_regularisation = regularizationFactor;
    }
  }
#endif  
  
}


