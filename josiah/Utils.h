#pragma once

#include <vector>
#include "Timer.h"
#include "GainFunction.h"
#include "FeatureFunction.h"

namespace Josiah {
  
  class GainFunctionVector;
  
  /**
   * Wrap moses timer to give a way to no-op it.
   **/
  class GibbsTimer {
  public:
    GibbsTimer() : m_doTiming(false) {}
    void on() {m_doTiming = true; m_timer.start("TIME: Starting timer");}
    void check(const string& msg) {if (m_doTiming) m_timer.check(string("TIME:" + msg).c_str());}
  private:
    Timer m_timer;
    bool m_doTiming;
  } ;
  
  void configure_features_from_file(const std::string& filename, feature_vector& fv);
  void LoadReferences(const vector<string>& ref_files, string input_file, GainFunctionVector* g, float bp_scale = 1.0, bool use_bp_denum_hack = false);
}

