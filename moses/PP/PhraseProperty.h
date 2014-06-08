#pragma once

#include <string>
#include <iostream>

namespace Moses
{

/** base class for all phrase properties.
 */
class PhraseProperty
{
public:
  PhraseProperty(const std::string &value) : m_value(value) {};

  virtual void ProcessValue() {};

  const std::string &GetValueString() {
    return m_value;
  };

protected:

  const std::string m_value;

};

} // namespace Moses

