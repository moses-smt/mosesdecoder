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
      PerceptronLearner(const Moses::ScoreComponentCollection& initWeights, const std::string& name, float learning_rate = 1.0) : OnlineLearner(initWeights, name), m_learning_rate(learning_rate), m_numUpdates() {}
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
    MiraLearner(const Moses::ScoreComponentCollection& initWeights,  const std::string& name, bool fixMargin, float margin, float slack) : OnlineLearner(initWeights, name), m_numUpdates(), m_fixMargin(fixMargin), m_margin(margin), m_slack(slack) { std::cerr << "Slack " << m_slack << std::endl;}
      virtual void doUpdate(TranslationDelta* curr, TranslationDelta* target, TranslationDelta* noChangeDelta, Sampler& sampler);
      virtual ~MiraLearner() {}
      virtual void reset() {m_numUpdates = 0;}
      virtual size_t GetNumUpdates() { return m_numUpdates;} 
    protected:
      size_t m_numUpdates;
      bool m_fixMargin;
    float m_margin;
    float m_slack;
  };
  
  class MiraPlusLearner : public MiraLearner {
    public :
      MiraPlusLearner(const Moses::ScoreComponentCollection& initWeights, const std::string& name, bool fixMargin, float margin, float slack) : MiraLearner(initWeights, name, fixMargin, margin, slack) {}
      virtual void doUpdate(TranslationDelta* curr, TranslationDelta* target, TranslationDelta* noChangeDelta, Sampler& sampler);
      virtual ~MiraPlusLearner() {}
  };
}
