#pragma once

#include <string>

#include "moses/TranslationModel/PhraseDictionary.h"

namespace Moses
{

class ChartParser;
class ChartCellCollectionBase;
class AllOptions;
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

  void Load(AllOptions::ptr const& opts);

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

  // Get the source terminal vocabulary for this table's grammar (as a set of
  // factor IDs)
  const boost::unordered_set<std::size_t> &GetSourceTerminalSet() const {
    return m_sourceTerminalSet;
  }

private:
  static std::vector<RuleTableFF*> s_instances;

  const RuleTable *m_table;
  boost::unordered_set<std::size_t> m_sourceTerminalSet;
};

}  // Syntax
}  // Moses
