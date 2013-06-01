//Fabienne Braune
//In memory version of l-MBOT dotted rules

#pragma once

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "DotChartMBOT.h"
#include "DotChartInMemoryMBOT.h"
#include "DotChartInMemory.h"
#include "moses/TranslationModel/RuleTable/PhraseDictionaryNodeMBOT.h"

#include "util/check.hh"
#include <vector>

namespace Moses
{

class DottedRuleInMemoryMBOT : public DottedRuleMBOT, public DottedRuleInMemory
{
 public:
  // used only to init dot stack.
  explicit DottedRuleInMemoryMBOT(const PhraseDictionaryNodeMBOT &node)
    :   DottedRuleMBOT()
     , DottedRuleInMemory(node)
      , m_mbotNode(node) {}

  DottedRuleInMemoryMBOT(const PhraseDictionaryNodeMBOT &node,
                     const ChartCellLabelMBOT &cellLabel,
                     const DottedRuleInMemoryMBOT &prev)
      : DottedRuleMBOT(cellLabel, prev)
      , DottedRuleInMemory(node, cellLabel, prev)
      , m_mbotNode(node) {}

  ~DottedRuleInMemoryMBOT()
  {
	  std::cerr << "KILLING DOTTED RULE IN MEMORY MBOT..." << std::endl;
  }

  const PhraseDictionaryNodeMBOT &GetLastNode() const {return m_mbotNode;}

 private:
  const PhraseDictionaryNodeMBOT &m_mbotNode;
};

//Fabienne Braune : Here we define a collection of in memory dotted rules
typedef std::vector<const DottedRuleInMemoryMBOT*> DottedRuleListMBOT;

// Collection of all in-memory l-MBOT DottedRules that share a common start point,
// grouped by end point.  Additionally, maintains a list of all
// DottedRules that could be expanded further, i.e. for which the
// corresponding PhraseDictionaryNodeSCFG is not a leaf.
class DottedRuleCollMBOT //: DottedRuleColl
{
protected:
  typedef std::vector<DottedRuleListMBOT> CollTypeMBOT;
  CollTypeMBOT m_mbotColl;
  DottedRuleListMBOT m_mbotExpandableDottedRuleList;

public:
  typedef CollTypeMBOT::iterator iterator;
  typedef CollTypeMBOT::const_iterator const_iterator;

  const_iterator begin() const {
    return m_mbotColl.begin();
  }
  const_iterator end() const {
    return m_mbotColl.end();
  }
  iterator begin() {
    return m_mbotColl.begin();
  }
  iterator end() {
    return m_mbotColl.end();
  }

  DottedRuleCollMBOT(size_t size)
    : m_mbotColl(size)
  {}

  ~DottedRuleCollMBOT();

  const DottedRuleListMBOT &GetMBOT(size_t pos) const {
    return m_mbotColl[pos];
  }
  DottedRuleListMBOT &GetMBOT(size_t pos) {
    return m_mbotColl[pos];
  }

 size_t GetSizeMBOT(){
 return m_mbotColl.size();
 }

  void Add(size_t pos, const DottedRuleInMemoryMBOT *dottedRule) {
    CHECK(dottedRule);
    m_mbotColl[pos].push_back(dottedRule);
    if (!dottedRule->GetLastNode().IsLeafMBOT()) {
      m_mbotExpandableDottedRuleList.push_back(dottedRule);
    }
  }

  void Clear(size_t pos) {
	  std::cerr << "CLEARING DOT CHART IN MEMORY..." << std::endl;
#ifdef USE_BOOST_POOL
    m_mbotColl[pos].clear();
#endif
  }

  const DottedRuleListMBOT &GetExpandableDottedRuleListMBOT() const {
	std::cerr << "Getting expandable rule list" << std::endl;
    return m_mbotExpandableDottedRuleList;
  }

};


}
