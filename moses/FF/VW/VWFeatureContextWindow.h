#pragma once

#include <string>
#include <algorithm>
#include "VWFeatureContext.h"
#include "moses/Util.h"

namespace Moses
{

class VWFeatureContextWindow : public VWFeatureContext
{
public:
  VWFeatureContextWindow(const std::string &line)
    : VWFeatureContext(line, DEFAULT_WINDOW_SIZE) {
    ReadParameters();

    // Call this last
    VWFeatureBase::UpdateRegister();
  }

  virtual void operator()(const Phrase &phrase
                          , std::vector<std::string> &features) const {
    for (size_t i = 0; i < m_contextSize; i++)
      features.push_back("tcwin^-" + SPrint(i + 1) + "^" + GetWord(phrase, i));
  }

  virtual void SetParameter(const std::string& key, const std::string& value) {
    if (key == "size") {
      m_contextSize = Scan<size_t>(value);
    } else {
      VWFeatureContext::SetParameter(key, value);
    }
  }

private:
  static const int DEFAULT_WINDOW_SIZE = 1;
};

}
