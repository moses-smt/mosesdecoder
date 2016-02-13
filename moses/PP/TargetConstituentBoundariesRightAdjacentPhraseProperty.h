#pragma once

#include "moses/PP/PhraseProperty.h"
#include "moses/Factor.h"
#include "util/exception.hh"
#include <map>
#include <string>

namespace Moses
{

typedef std::map<const Factor*, float> TargetConstituentBoundariesRightAdjacentCollection;


class TargetConstituentBoundariesRightAdjacentPhraseProperty : public PhraseProperty
{
public:
  TargetConstituentBoundariesRightAdjacentPhraseProperty()
  {};

  virtual void ProcessValue(const std::string &value);

  const TargetConstituentBoundariesRightAdjacentCollection &GetCollection() const {
    return m_constituentsCollection;
  };

  virtual const std::string *GetValueString() const {
    UTIL_THROW2("TargetConstituentBoundariesRightAdjacentPhraseProperty: value string not available in this phrase property");
    return NULL;
  };

protected:

  virtual void Print(std::ostream& out) const;

  TargetConstituentBoundariesRightAdjacentCollection m_constituentsCollection;
};

} // namespace Moses

