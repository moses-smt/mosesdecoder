#include "CorpusSamplerAnnealed.h"

#include "Hypothesis.h"
#include "Derivation.h"
#ifdef MPI_ENABLED
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/collectives.hpp>
namespace mpi=boost::mpi;
#endif

using namespace std;

namespace Josiah {
  
  FVector CorpusSamplerAnnealedCollector::getExpectedFeatureValue(std::map<const Derivation*,double>& m_p) {
    FVector expFV;
    for (std::map<const Derivation*,double>::const_iterator it = m_p.begin(); it != m_p.end(); ++it) {
     const Derivation* deriv = it->first;
      expFV += (deriv->getFeatureValues() * (it->second));
    }
    return expFV;
  }
  
  void CorpusSamplerAnnealedCollector::setRegularisationGradientFactor(std::map<const Derivation*,double>& m_p) {
    double temperature = GetTemperature();
    FVector expFV = getExpectedFeatureValue(m_p);
    //cerr << "Expected FV " << expFV << endl;
    float entropy_factor;
    for (std::map<const Derivation*,double>::const_iterator it = m_p.begin(); it != m_p.end(); ++it) {
      entropy_factor = -temperature * it->second * (log (it->second)+1);
      //cerr << "Entropy factor " << entropy_factor << endl;
      FVector fv = it->first->getFeatureValues();  
      fv -= expFV;
      fv *= entropy_factor;
      m_gradient += fv;
    }
    //cerr << "Gradient regularization " << m_gradient << endl;
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
    AggregateRegularisationStats(rank);
    AggregateSuffStats(rank);
  }
  
  void CorpusSamplerAnnealedCollector::AggregateRegularisationStats(int rank) {  
    FVector regularizationGrad;
    float regularizationFactor;
    FVector recvRegGrad;
    
    //Reduce regularization
    float reg = getRegularisation();
    MPI_VERBOSE(1, "Regualarization for rank " << rank << " = " << reg << endl); 
    mpi::communicator world;
    mpi::reduce(world,reg,regularizationFactor,std::plus<float>(),0);
    //if (MPI_SUCCESS != MPI_Reduce(&reg, &regularizationFactor, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    
    //Reduce regularization gradient
    MPI_VERBOSE(1, "Regualarization grad for rank " << rank << " = " << getRegularisationGradientFactor() << endl);
    mpi::reduce(world,getRegularisationGradientFactor(), m_gradient ,FVectorPlus(),0);
    //if (MPI_SUCCESS != MPI_Reduce(const_cast<float*>(&regGrad[0]), &recvRegGrad[0], regGrad.size(), MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    
    if (rank == 0 ) {  
      m_regularisation = regularizationFactor;
    }
    MPI_VERBOSE(1, "After agg, Regualarization for rank " << rank << " = " << m_regularisation << endl);
    MPI_VERBOSE(1, "After agg, Regualarization grad for rank " << rank << " = " << m_gradient << endl);
  }
#endif  
  
}


