
#pragma once

#include "moses/PP/PhraseProperty.h"
#include "util/exception.hh"
#include <string>
#include <list>

namespace Moses
{

//    A simple phrase property class to access the three phrase count values.
//
//    The counts are usually not needed during decoding and are not loaded
//    from the phrase table. This is just a workaround that can make them
//    available to features which have a use for them.
//
//    If you need access to the counts, copy the two marginal counts and the
//    joint count into an additional information property with key "Counts",
//    e.g. using awk:
//
//    $ zcat phrase-table.gz | awk -F' \|\|\| '  '{printf("%s {{Counts %s}}\n",$0,$5);}' | gzip -c > phrase-table.withCountsPP.gz
//
//    CountsPhraseProperty reads them from the phrase table and provides
//    methods GetSourceMarginal(), GetTargetMarginal(), GetJointCount().


class CountsPhraseProperty : public PhraseProperty
{
  friend std::ostream& operator<<(std::ostream &, const CountsPhraseProperty &);

public:

  CountsPhraseProperty() {};

  virtual void ProcessValue(const std::string &value);

  size_t GetSourceMarginal() const {
    return m_sourceMarginal;
  }

  size_t GetTargetMarginal() const {
    return m_targetMarginal;
  }

  float GetJointCount() const {
    return m_jointCount;
  }

  virtual const std::string *GetValueString() const {
    UTIL_THROW2("CountsPhraseProperty: value string not available in this phrase property");
    return NULL;
  };

protected:

  float m_sourceMarginal, m_targetMarginal, m_jointCount;

};

} // namespace Moses

