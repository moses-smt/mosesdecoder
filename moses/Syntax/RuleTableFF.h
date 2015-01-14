#pragma once

#include <string>

#include "moses/TranslationModel/PhraseDictionary.h"

namespace Moses
{

class ChartParser;
class ChartCellCollectionBase;

namespace Syntax
{

class RuleTable;

// Feature function for dealing with local rule scores (that come from a
// rule table).  The scores themselves are stored on TargetPhrase objects
// and the decoder accesses them directly, so this object doesn't really do
// anything except provide somewhere to store the weights and parameter values.
class RuleTableFF : public PhraseDictionary
{
public:
  RuleTableFF(const std::string &);

  // FIXME Delete m_table?
  ~RuleTableFF() {}

  void Load();

  const RuleTable *GetTable() const {
    return m_table;
  }

  static const std::vector<RuleTableFF*> &Instances() {
    return s_instances;
  }

  ChartRuleLookupManager *CreateRuleLookupManager(
    const ChartParser &, const ChartCellCollectionBase &, std::size_t) {
    assert(false);
    return 0;
  }

private:
  static std::vector<RuleTableFF*> s_instances;

  const RuleTable *m_table;
};

}  // Syntax
}  // Moses
