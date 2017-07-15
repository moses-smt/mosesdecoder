#pragma once

#include <string>
#include <iostream>

namespace Moses
{

/** base class for all phrase properties.
 */
class PhraseProperty
{
  friend std::ostream& operator<<(std::ostream &, const PhraseProperty &);

public:
  PhraseProperty() : m_value(NULL) {};
  virtual ~PhraseProperty() {
    if ( m_value != NULL ) delete m_value;
  };

  virtual void ProcessValue(const std::string &value) {
    m_value = new std::string(value);
  };

  virtual const std::string *GetValueString() const {
    return m_value;
  };

protected:

  virtual void Print(std::ostream& out) const;

  std::string *m_value;

};

} // namespace Moses

