
#pragma once

#include "moses/PP/PhraseProperty.h"
#include <string>
#include <list>

namespace Moses
{

class CountsPhraseProperty : public PhraseProperty
{
public:

  CountsPhraseProperty(const std::string &value) :  PhraseProperty(value) {};

  virtual void ProcessValue();

  size_t GetSourceMarginal() const {
    return m_sourceMarginal;
  }

  size_t GetTargetMarginal() const {
    return m_targetMarginal;
  }

  float GetJointCount() const {
    return m_jointCount;
  }

protected:

  float m_sourceMarginal, m_targetMarginal, m_jointCount;

};

} // namespace Moses

