#pragma once

#include "moses/PP/PhraseProperty.h"
#include "moses/Factor.h"
#include "util/exception.hh"
#include <map>
#include <string>

namespace Moses
{

typedef std::map<const Factor*, float> TargetConstituentBoundariesLeftCollection;


class TargetConstituentBoundariesLeftPhraseProperty : public PhraseProperty
{
public:
  TargetConstituentBoundariesLeftPhraseProperty()
  {};

  virtual void ProcessValue(const std::string &value);

  const TargetConstituentBoundariesLeftCollection &GetCollection() const {
    return m_constituentsCollection;
  };

  virtual const std::string *GetValueString() const {
    UTIL_THROW2("TargetConstituentBoundariesLeftPhraseProperty: value string not available in this phrase property");
    return NULL;
  };

protected:

  virtual void Print(std::ostream& out) const;

  TargetConstituentBoundariesLeftCollection m_constituentsCollection;
};

} // namespace Moses

