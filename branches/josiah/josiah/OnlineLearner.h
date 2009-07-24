#pragma once
#include <iostream>
#include <iomanip>
#include <fstream>

#include "ScoreComponentCollection.h"

namespace Josiah {
  class TranslationDelta; 
  class Sampler;
  
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
      PerceptronLearner(const Moses::ScoreComponentCollection& initWeights, const std::string& name, float learning_rate = 1.0) : OnlineLearner(initWeights, "PERCEPTRON"), m_learning_rate(learning_rate), m_numUpdates() {}
      virtual void doUpdate(TranslationDelta* curr, TranslationDelta* target, TranslationDelta* noChangeDelta, Sampler& sampler);
      virtual ~PerceptronLearner() {}
      virtual void reset() {m_numUpdates = 0;}
      virtual size_t GetNumUpdates() { return m_numUpdates;}
    private:
      float m_learning_rate;
      size_t m_numUpdates;
  };
  
  class MiraLearner : public OnlineLearner {
    public :
      MiraLearner(const Moses::ScoreComponentCollection& initWeights, const std::string& name, float learning_rate = 1.0) : OnlineLearner(initWeights, "MIRA"), m_learning_rate(learning_rate), m_numUpdates() {}
      virtual void doUpdate(TranslationDelta* curr, TranslationDelta* target, TranslationDelta* noChangeDelta, Sampler& sampler);
      virtual ~MiraLearner() {}
      virtual void reset() {m_numUpdates = 0;}
      virtual size_t GetNumUpdates() { return m_numUpdates;} 
    private:
      float m_learning_rate;
      size_t m_numUpdates;
  };
  
  class MiraPlusLearner : public OnlineLearner {
    public :
      MiraPlusLearner(const Moses::ScoreComponentCollection& initWeights, const std::string& name, float learning_rate = 1.0) : OnlineLearner(initWeights, "MIRA++"), m_learning_rate(learning_rate), m_numUpdates() {}
      virtual void doUpdate(TranslationDelta* curr, TranslationDelta* target, TranslationDelta* noChangeDelta, Sampler& sampler);
      virtual ~MiraPlusLearner() {}
      virtual void reset() {m_numUpdates = 0;}
      virtual size_t GetNumUpdates() { return m_numUpdates;} 
    private:
      float m_learning_rate;
      size_t m_numUpdates;
  };
}
