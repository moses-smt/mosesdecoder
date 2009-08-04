#pragma once
#include <iostream>
#include <iomanip>
#include <fstream>
#ifdef MPI_ENABLED
#include <mpi.h>
#endif


#include "ScoreComponentCollection.h"

namespace Josiah {
  class TranslationDelta; 
  class Sampler;
  class WeightNormalizer;
  
  class OnlineLearner {
    public :	
			OnlineLearner(const Moses::ScoreComponentCollection& initWeights, const std::string& name) : m_currWeights(initWeights), m_cumulWeights(initWeights), m_name(name), m_iteration(0) {} //, m_averaging(true)
      virtual void doUpdate(TranslationDelta* curr, TranslationDelta* target, TranslationDelta* noChangeDelta, Sampler& sampler)  = 0;
      void UpdateCumul() ;
      const Moses::ScoreComponentCollection& GetCurrWeights() { return m_currWeights; }
      Moses::ScoreComponentCollection GetAveragedWeights() ;
      virtual ~OnlineLearner() {}
      virtual void reset() {}
      virtual size_t GetNumUpdates() = 0;
      const std::string & GetName() {return m_name;}
#ifdef MPI_ENABLED    
      void SetRunningWeightVector(int, int);
#endif
    protected:
      //bool m_averaging;
      Moses::ScoreComponentCollection m_currWeights;
      Moses::ScoreComponentCollection m_cumulWeights;
      std::string m_name;
      size_t m_iteration;
      std::vector<float> hildreth ( const std::vector<Moses::ScoreComponentCollection>& a, const std::vector<float>& b );
      std::vector<float> hildreth ( const std::vector<Moses::ScoreComponentCollection>& a, const std::vector<float>& b, float );
  };

  class PerceptronLearner : public OnlineLearner {
    public :
      PerceptronLearner(const Moses::ScoreComponentCollection& initWeights, const std::string& name, float learning_rate = 1.0) : OnlineLearner(initWeights, name), m_learning_rate(learning_rate), m_numUpdates() {}
      virtual void doUpdate(TranslationDelta* curr, TranslationDelta* target, TranslationDelta* noChangeDelta, Sampler& sampler);
      virtual ~PerceptronLearner() {}
      virtual void reset() {m_numUpdates = 0;}
      virtual size_t GetNumUpdates() { return m_numUpdates;}
    private:
      float m_learning_rate;
      size_t m_numUpdates;
  };
  
	class CWLearner : public OnlineLearner {
		public :
		CWLearner(const Moses::ScoreComponentCollection& initWeights, const std::string& name, float confidence = 1.644854f, float initialVariance = 1.0f) : 
			OnlineLearner(initWeights, name), m_confidence(confidence), m_numUpdates(), m_epsilon(0.0000001f),m_currSigmaDiag(initWeights),m_features(initWeights) {
				float size = m_currSigmaDiag.data().size();
				for (size_t i=0; i<size; i++) {
					m_currSigmaDiag[i] = initialVariance;
				} 							
			}
		virtual void doUpdate(TranslationDelta* curr, TranslationDelta* target, TranslationDelta* noChangeDelta, Sampler& sampler);
		virtual ~CWLearner() {}
		virtual void reset() {m_numUpdates = 0;}
		virtual size_t GetNumUpdates() { return m_numUpdates;}
    private:
		Moses::ScoreComponentCollection m_currSigmaDiag;
		Moses::ScoreComponentCollection m_features;
		float m_confidence;
		float m_learning_rate;
		float m_epsilon;
		size_t m_numUpdates;	
		
		bool sign(float value) { return value > 0.0; } 
			
		float kkt(float marginMean, float marginVariance) {
			if (marginMean >= m_confidence * marginVariance) return 0.0;
			//margin variance approximately == 0 ? 
			if (marginVariance < 0.0 + m_epsilon && marginVariance > 0.0 - m_epsilon) return 0.0;
			float v	= 1.0 + 2.0 * m_confidence * marginMean;
			float lambda = -v + sqrt(v * v - 8.0 * m_confidence * (marginMean - m_confidence * marginVariance)) / (4.0 * m_confidence * marginVariance);
			return lambda > 0.0 ? lambda : 0.0;
		}
		
		float calculateMarginVariance(const Moses::ScoreComponentCollection& features) { 
			float sum = 0.0;
			float size = features.data().size();
			for (size_t i=0; i<size; i++) { sum += features.data()[i] * features.data()[i] * m_currSigmaDiag.data()[i]; } 
			return sum;
		}
		
		void updateMean(float alpha, float y) {
			float size = m_currWeights.data().size();
			for (size_t i=0; i<size; i++) { 
				m_currWeights[i] += alpha * y * m_currSigmaDiag[i] * m_features[i];
			} 			
		}

		void updateVariance(float alpha) {
			float size = m_currSigmaDiag.data().size();
			for (size_t i=0; i<size; i++) {
				m_currSigmaDiag[i] += 1.0 / (1.0/m_currSigmaDiag[i] + 2.0 * alpha *	m_confidence * m_features[i]);
			} 			
		}
		
		
	}; 
	
	
  class MiraLearner : public OnlineLearner {
    public :
    MiraLearner(const Moses::ScoreComponentCollection& initWeights,  const std::string& name, bool fixMargin, float margin, float slack, WeightNormalizer* wn = NULL) : OnlineLearner(initWeights, name), m_numUpdates(), m_fixMargin(fixMargin), m_margin(margin), m_slack(slack), m_normalizer(wn) {}
      virtual void doUpdate(TranslationDelta* curr, TranslationDelta* target, TranslationDelta* noChangeDelta, Sampler& sampler);
      virtual ~MiraLearner() {}
      virtual void reset() {m_numUpdates = 0;}
      virtual size_t GetNumUpdates() { return m_numUpdates;} 
      void SetNormalizer(WeightNormalizer* normalizer) {m_normalizer = normalizer;}
    protected:
      size_t m_numUpdates;
      bool m_fixMargin;
      float m_margin;
      float m_slack;
      WeightNormalizer* m_normalizer;
  };
  
  class MiraPlusLearner : public MiraLearner {
    public :
      MiraPlusLearner(const Moses::ScoreComponentCollection& initWeights, const std::string& name, bool fixMargin, float margin, float slack, WeightNormalizer* wn = NULL) : MiraLearner(initWeights, name, fixMargin, margin, slack, wn) {}
      virtual void doUpdate(TranslationDelta* curr, TranslationDelta* target, TranslationDelta* noChangeDelta, Sampler& sampler);
      virtual ~MiraPlusLearner() {}
  };
  
  class WeightNormalizer {
    public :
      WeightNormalizer(float norm) {m_norm = norm;}
      virtual ~WeightNormalizer() {}
      virtual void Normalize(Moses::ScoreComponentCollection& ) = 0; 
    protected :
      float m_norm;
  };
  
  class L1Normalizer : public WeightNormalizer {
    public:
      L1Normalizer (float norm) : WeightNormalizer(norm) {}
      virtual ~L1Normalizer() {}
      virtual void Normalize(Moses::ScoreComponentCollection& weights) {
        float currNorm = weights.GetL1Norm();
        weights.MultiplyEquals(m_norm/currNorm);
      } 
  };
  
  class L2Normalizer : public WeightNormalizer {
    public:
      L2Normalizer (float norm) : WeightNormalizer(norm) {}
      virtual ~L2Normalizer() {}
      virtual void Normalize(Moses::ScoreComponentCollection& weights) {
        float currNorm = weights.GetL2Norm();
        weights.MultiplyEquals(m_norm/currNorm);
      } 
  };
}
