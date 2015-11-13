
#pragma once

#include "moses/PP/PhraseProperty.h"
#include "util/exception.hh"
#include <string>
#include <list>

namespace Moses
{

// Note that we require label tokens (strings) in the corresponding property values of phrase table entries
// to be replaced beforehand by indices (size_t) of a label vocabulary. (TODO: change that?)

class SourceLabelsPhrasePropertyItem
{
  friend class SourceLabelsPhraseProperty;

public:
  SourceLabelsPhrasePropertyItem() {};

  float GetSourceLabelsRHSCount() const {
    return m_sourceLabelsRHSCount;
  };

  const std::list<size_t> &GetSourceLabelsRHS() const {
    return m_sourceLabelsRHS;
  };

  const std::list< std::pair<size_t,float> > &GetSourceLabelsLHSList() const {
    return m_sourceLabelsLHSList;
  };

private:
  float m_sourceLabelsRHSCount;
  std::list<size_t> m_sourceLabelsRHS; // should be of size nNTs-1 (empty if initial rule, i.e. no right-hand side non-terminals)
  std::list< std::pair<size_t,float> > m_sourceLabelsLHSList; // list of left-hand sides for this right-hand side, with counts
};


class SourceLabelsPhraseProperty : public PhraseProperty
{
public:
  SourceLabelsPhraseProperty() {};

  virtual void ProcessValue(const std::string &value);

  size_t GetNumberOfNonTerminals() const {
    return m_nNTs;
  }

  float GetTotalCount() const {
    return m_totalCount;
  }

  const std::list<SourceLabelsPhrasePropertyItem> &GetSourceLabelItems() const {
    return m_sourceLabelItems;
  };

  virtual const std::string *GetValueString() const {
    UTIL_THROW2("SourceLabelsPhraseProperty: value string not available in this phrase property");
    return NULL;
  };

protected:

  size_t m_nNTs;
  float m_totalCount;

  std::list<SourceLabelsPhrasePropertyItem> m_sourceLabelItems;
};

} // namespace Moses

