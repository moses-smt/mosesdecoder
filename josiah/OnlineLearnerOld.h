#pragma once

#include <cmath>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#ifdef MPI_ENABLED
#include <mpi.h>
#endif

#include "FeatureVector.h"
#include "Gain.h"
#include "TranslationDelta.h"
#include "WeightManager.h"

namespace Josiah {
  class Sampler;
  class WeightNormalizer;
  
  class OnlineLearner {
    public :	
			OnlineLearner(const FVector& initWeights, const std::string& name) : 
               m_name(name), m_iteration(0) {} //, m_averaging(true)
            virtual void doUpdate(const TDeltaHandle& curr, 
                                  const TDeltaHandle& target, 
                                  const TDeltaHandle& noChangeDelta, 
                                  const FVector& optimalFV,
                                  const FValue optimalGain, 
                                  const GainFunctionHandle& gf)  = 0;
      void UpdateCumul() ;
      FVector& GetCurrWeights() {return WeightManager::instance().get();}
      FVector GetAveragedWeights() ;
      virtual ~OnlineLearner() {}
      virtual void reset() {}
      virtual size_t GetNumUpdates() = 0;
      const std::string & GetName() {return m_name;}
    protected:
      //bool m_averaging;
      FVector m_cumulWeights;
      std::string m_name;
      size_t m_iteration;
      std::vector<FValue> hildreth ( const std::vector<FVector>& a, const std::vector<FValue>& b );
      std::vector<FValue> hildreth ( const std::vector<FVector>& a, const std::vector<FValue>& b, FValue );
  };

  class PerceptronLearner : public OnlineLearner {
    public :
      PerceptronLearner(const FVector& initWeights, const std::string& name, FValue learning_rate = 1.0) : OnlineLearner(initWeights, name), m_learning_rate(learning_rate), m_numUpdates() {}
      virtual void doUpdate(const TDeltaHandle& curr, 
                            const TDeltaHandle& target, 
                            const TDeltaHandle& noChangeDelta, 
                            const FVector& optimalFV,
                            const FValue optimalGain, 
                            const GainFunctionHandle& gf);
      virtual ~PerceptronLearner() {}
      virtual void reset() {m_numUpdates = 0;}
      virtual size_t GetNumUpdates() { return m_numUpdates;}
    private:
      FValue m_learning_rate;
      size_t m_numUpdates;
  };
  
	class CWLearner : public OnlineLearner {
		public :
                  CWLearner(const FVector& initWeights, const std::string& name, FValue confidence = 1.644854f, FValue initialVariance = 1.0f) :
                      OnlineLearner(initWeights, name),   m_features(initWeights),m_confidence(confidence),  m_epsilon(0.0000001f),m_numUpdates(){
                m_currSigmaDiag += initialVariance;						
			}
            virtual void doUpdate(const TDeltaHandle& curr, 
                                  const TDeltaHandle& target, 
                                  const TDeltaHandle& noChangeDelta, 
                                  const FVector& optimalFV,
                                  const FValue optimalGain, 
                                  const GainFunctionHandle& gf);
		virtual ~CWLearner() {}
		virtual void reset() {m_numUpdates = 0;}
		virtual size_t GetNumUpdates() { return m_numUpdates;}
    private:
		FVector m_currSigmaDiag;
		FVector m_features;
		FValue m_confidence;
		FValue m_learning_rate;
		FValue m_epsilon;
		size_t m_numUpdates;	
		
		bool sign(FValue value) { return value > 0.0; } 
			
		FValue kkt(FValue marginMean, FValue marginVariance) {
			if (marginMean >= m_confidence * marginVariance) return 0.0;
			//margin variance approximately == 0 ? 
			if (marginVariance < 0.0 + m_epsilon && marginVariance > 0.0 - m_epsilon) return 0.0;
			FValue v	= 1.0 + 2.0 * m_confidence * marginMean;
			FValue lambda = (-v + sqrt(v * v - 8.0 * m_confidence * (marginMean - m_confidence * marginVariance))) / (4.0 * m_confidence * marginVariance);
			return lambda > 0.0 ? lambda : 0.0;
		}
		
		FValue calculateMarginVariance(const FVector& features) { 
          return (features*features*m_currSigmaDiag).sum();
		}
		
        void updateMean(FValue alpha, FValue y) {
            WeightManager::instance().get() += alpha*y*m_currSigmaDiag*m_features;
		}

        void updateVariance(FValue alpha) {		
            m_currSigmaDiag = 1.0 / (1.0 / m_currSigmaDiag + (2.0 * alpha * m_confidence * m_features * m_features));
		}
		
		
	}; 
	
	
  class MiraLearner : public OnlineLearner {
    public :
    MiraLearner(const FVector& initWeights,  const std::string& name, bool fixMargin, FValue margin, FValue slack, FValue scale_margin = 1.0, WeightNormalizer* wn = NULL) : OnlineLearner(initWeights, name), m_numUpdates(), m_fixMargin(fixMargin), m_margin(margin), m_slack(slack), m_marginScaleFactor(scale_margin), m_normalizer(wn) {}
    virtual void doUpdate(const TDeltaHandle& curr, 
                          const TDeltaHandle& target, 
                          const TDeltaHandle& noChangeDelta, 
                          const FVector& optimalFV,
                          const FValue optimalGain, 
                          const GainFunctionHandle& gf);
      virtual ~MiraLearner() {}
      virtual void reset() {m_numUpdates = 0;}
      virtual size_t GetNumUpdates() { return m_numUpdates;} 
      void SetNormalizer(WeightNormalizer* normalizer) {m_normalizer = normalizer;}
    protected:
      size_t m_numUpdates;
      bool m_fixMargin;
      FValue m_margin;
      FValue m_slack;  
      FValue m_marginScaleFactor;
      WeightNormalizer* m_normalizer;
  };
  
  class MiraPlusLearner : public MiraLearner {
    public :
      MiraPlusLearner(const FVector& initWeights, const std::string& name, bool fixMargin, FValue margin, FValue slack, FValue scale_margin = 1.0, WeightNormalizer* wn = NULL) : MiraLearner(initWeights, name, fixMargin, margin, slack, scale_margin, wn) {}
      virtual void doUpdate(const TDeltaHandle& curr, 
                            const TDeltaHandle& target, 
                            const TDeltaHandle& noChangeDelta, 
                            const FVector& optimalFV,
                            const FValue optimalGain, 
                            const GainFunctionHandle& gf);
      virtual ~MiraPlusLearner() {}
  };
  
  class WeightNormalizer {
    public :
      WeightNormalizer(FValue norm) {m_norm = norm;}
      virtual ~WeightNormalizer() {}
      virtual void Normalize(FVector& ) = 0; 
    protected :
      FValue m_norm;
  };
  
  class L1Normalizer : public WeightNormalizer {
    public:
      L1Normalizer (FValue norm) : WeightNormalizer(norm) {}
      virtual ~L1Normalizer() {}
      virtual void Normalize(FVector& weights) {
        weights *= (m_norm / weights.l1norm());
      } 
  };
  
  class L2Normalizer : public WeightNormalizer {
    public:
      L2Normalizer (FValue norm) : WeightNormalizer(norm) {}
      virtual ~L2Normalizer() {}
      virtual void Normalize(FVector& weights) {
        weights *= (m_norm / weights.l2norm());
      } 
  };
}
