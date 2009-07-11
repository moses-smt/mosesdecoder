#pragma once
#include <iostream>
#include <iomanip>
#include <fstream>

#include "TranslationDelta.h"
#include "ScoreComponentCollection.h"

namespace Josiah {
  
  
  class OnlineLearner {
    public :
      OnlineLearner(const ScoreComponentCollection& initWeights, const std::string& name) : m_currWeights(initWeights), m_cumulWeights(initWeights), m_name(name), m_iteration(0) {} //, m_averaging(true)
      virtual void doUpdate(TranslationDelta* curr, TranslationDelta* target)  = 0;
      //void setAveraging(bool averaging) { m_averaging = averaging; }
      const ScoreComponentCollection& GetCurrWeights() { return m_currWeights; }
      ScoreComponentCollection GetAveragedWeights() ;
    protected:
      std::string m_name;
      size_t m_iteration;
      //bool m_averaging;
      ScoreComponentCollection m_currWeights;
      ScoreComponentCollection m_cumulWeights;
  };

  class PerceptronLearner : public OnlineLearner {
    public :
    PerceptronLearner(const ScoreComponentCollection& initWeights, const std::string& name, float learning_rate = 1.0) : OnlineLearner(initWeights, name), m_learning_rate(learning_rate) {}
    virtual void doUpdate(TranslationDelta* curr, TranslationDelta* target);
    private:
      float m_learning_rate;
  };
  
  class MiraLearner : public OnlineLearner {
    public :
    MiraLearner(const ScoreComponentCollection& initWeights, const std::string& name, float learning_rate = 1.0) : OnlineLearner(initWeights, name), m_learning_rate(learning_rate) {}
    virtual void doUpdate(TranslationDelta* curr, TranslationDelta* target);
    private:
      float m_learning_rate;
      std::vector<float> hildreth ( const std::vector<ScoreComponentCollection>& a, const std::vector<float>& b );
  };
}