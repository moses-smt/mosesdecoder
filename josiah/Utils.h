#pragma once

#include <vector>
#include "Timer.h"
#include "FeatureFunction.h"

namespace Moses {
  class LanguageModel; 
}
namespace Josiah {
  
  
  
  /**
   * Wrap moses timer to give a way to no-op it.
   **/
  class GibbsTimer {
  public:
    GibbsTimer() : m_doTiming(false) {}
    void on() {m_doTiming = true; m_timer.start("TIME: Starting timer");}
    void check(const std::string& msg) {if (m_doTiming) m_timer.check(std::string("TIME:" + msg).c_str());}
  private:
    Timer m_timer;
    bool m_doTiming;
  } ;
  
  void configure_features_from_file(const std::string& filename, FeatureVector& fv, bool disableUWP, FVector& coreWeights);
//  bool ValidateAndGetLMFromName(std::string featsName, Moses::LanguageModel* &lm);
}

