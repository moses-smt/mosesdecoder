#include "moses/PP/OrientationPhraseProperty.h"
#include <iostream>


namespace Moses
{

void OrientationPhraseProperty::ProcessValue(const std::string &value)
{
  // bidirectional MSLR phrase orientation with 2x4 orientation classes: 
  // mono swap dright dleft

  std::istringstream tokenizer(value);

  try {
    if (! (tokenizer >> m_l2rMonoProbability >> m_l2rSwapProbability >> m_l2rDrightProbability >> m_l2rDleftProbability
                     >> m_r2lMonoProbability >> m_r2lSwapProbability >> m_r2lDrightProbability >> m_r2lDleftProbability)) {
      UTIL_THROW2("OrientationPhraseProperty: Not able to read value. Flawed property?");
    }
  } catch (const std::exception &e) {
    UTIL_THROW2("OrientationPhraseProperty: Read error. Flawed property?");
  }
};

} // namespace Moses

