
#pragma once

#include "moses/PP/PhraseProperty.h"
#include "util/exception.hh"
#include <string>
#include <list>

namespace Moses
{

class TargetPreferencesPhrasePropertyItem
{
  friend class TargetPreferencesPhraseProperty;

public:
  TargetPreferencesPhrasePropertyItem() {};

  float GetTargetPreferencesRHSCount() const {
    return m_labelsRHSCount;
  };

  const std::list<size_t> &GetTargetPreferencesRHS() const {
    return m_labelsRHS;
  };

  const std::list< std::pair<size_t,float> > &GetTargetPreferencesLHSList() const {
    return m_labelsLHSList;
  };

private:
  float m_labelsRHSCount;
  std::list<size_t> m_labelsRHS; // should be of size nNTs-1 (empty if initial rule, i.e. no right-hand side non-terminals)
  std::list< std::pair<size_t,float> > m_labelsLHSList; // list of left-hand sides for this right-hand side, with counts
};


class TargetPreferencesPhraseProperty : public PhraseProperty
{
public:
  TargetPreferencesPhraseProperty() {};

  virtual void ProcessValue(const std::string &value);

  size_t GetNumberOfNonTerminals() const {
    return m_nNTs;
  }

  float GetTotalCount() const {
    return m_totalCount;
  }

  const std::list<TargetPreferencesPhrasePropertyItem> &GetTargetPreferencesItems() const {
    return m_labelItems;
  };

  virtual const std::string *GetValueString() const {
    UTIL_THROW2("TargetPreferencesPhraseProperty: value string not available in this phrase property");
    return NULL;
  };

protected:

  size_t m_nNTs;
  float m_totalCount;

  std::list<TargetPreferencesPhrasePropertyItem> m_labelItems;
};

} // namespace Moses

