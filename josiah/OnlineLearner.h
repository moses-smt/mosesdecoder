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
      void UpdateCumul() ;
      //void setAveraging(bool averaging) { m_averaging = averaging; }
      const ScoreComponentCollection& GetCurrWeights() { return m_currWeights; }
      ScoreComponentCollection GetAveragedWeights() ;
      virtual ~OnlineLearner() {}
      virtual void reset() {}
      virtual size_t GetNumUpdates() = 0;
    protected:
      //bool m_averaging;
      ScoreComponentCollection m_currWeights;
      ScoreComponentCollection m_cumulWeights;
      std::string m_name;
      size_t m_iteration;
  };

  class PerceptronLearner : public OnlineLearner {
    public :
    PerceptronLearner(const ScoreComponentCollection& initWeights, const std::string& name, float learning_rate = 1.0) : OnlineLearner(initWeights, name), m_learning_rate(learning_rate), m_numUpdates() {}
    virtual void doUpdate(TranslationDelta* curr, TranslationDelta* target);
    virtual ~PerceptronLearner() {}
    virtual void reset() {m_numUpdates = 0;}
    virtual size_t GetNumUpdates() { return m_numUpdates;}
    private:
      float m_learning_rate;
      size_t m_numUpdates;
  };
  
  class MiraLearner : public OnlineLearner {
    public :
    MiraLearner(const ScoreComponentCollection& initWeights, const std::string& name, float learning_rate = 1.0) : OnlineLearner(initWeights, name), m_learning_rate(learning_rate), m_numUpdates() {}
    virtual void doUpdate(TranslationDelta* curr, TranslationDelta* target);
    virtual ~MiraLearner() {}
    virtual void reset() {m_numUpdates = 0;}
    virtual size_t GetNumUpdates() { return m_numUpdates;} 
    private:
      float m_learning_rate;
      std::vector<float> hildreth ( const std::vector<ScoreComponentCollection>& a, const std::vector<float>& b );
      size_t m_numUpdates;
  };
}
