
#pragma once

#include "moses/PP/PhraseProperty.h"
#include "util/exception.hh"
#include <string>

namespace Moses
{

class OrientationPhraseProperty : public PhraseProperty
{
public:
  OrientationPhraseProperty() {};

  virtual void ProcessValue(const std::string &value);


  double GetLeftToRightProbabilityMono() const {
    return m_l2rMonoProbability;
  };

  double GetLeftToRightProbabilitySwap() const {
    return m_l2rSwapProbability;
  };

  double GetLeftToRightProbabilityDleft() const {
    return m_l2rDleftProbability;
  };

  double GetLeftToRightProbabilityDright() const {
    return m_l2rDrightProbability;
  };

  double GetLeftToRightProbabilityDiscontinuous() const {
    return m_l2rDleftProbability + m_l2rDrightProbability;
  };


  double GetRightToLeftProbabilityMono() const {
    return m_r2lMonoProbability;
  };

  double GetRightToLeftProbabilitySwap() const {
    return m_r2lSwapProbability;
  };

  double GetRightToLeftProbabilityDleft() const {
    return m_r2lDleftProbability;
  };

  double GetRightToLeftProbabilityDright() const {
    return m_r2lDrightProbability;
  };

  double GetRightToLeftProbabilityDiscontinuous() const {
    return m_r2lDleftProbability + m_r2lDrightProbability;
  };


  virtual const std::string *GetValueString() const {
    UTIL_THROW2("OrientationPhraseProperty: value string not available in this phrase property");
    return NULL;
  };

protected:

  float m_l2rMonoProbability, m_l2rSwapProbability, m_l2rDrightProbability, m_l2rDleftProbability,
        m_r2lMonoProbability, m_r2lSwapProbability, m_r2lDrightProbability, m_r2lDleftProbability;
};

} // namespace Moses

