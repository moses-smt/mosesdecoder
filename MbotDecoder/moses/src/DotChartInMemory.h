/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/

#pragma once

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "DotChart.h"
#include "DotChartMBOT.h"
#include "PhraseDictionaryNodeSCFG.h"
#include "PhraseDictionaryNodeMBOT.h"

#include "util/check.hh"
#include <vector>

namespace Moses
{

class DottedRuleInMemory : public DottedRule
{
 public:
  // used only to init dot stack.
  explicit DottedRuleInMemory(const PhraseDictionaryNodeSCFG &node)
      : DottedRule()
      , m_node(node) {
          //std::cout<<"new DottedRuleInMemory() " << this << std::endl;
          }

  DottedRuleInMemory(const PhraseDictionaryNodeSCFG &node,
                     const ChartCellLabel &cellLabel,
                     const DottedRuleInMemory &prev)
      : DottedRule(cellLabel, prev)
    , m_node(node) {//std::cout<<"new DottedRuleInMemory(3) " << this << std::endl;
    }

  virtual const PhraseDictionaryNodeSCFG &GetLastNode() const { return m_node; }

 private:
  const PhraseDictionaryNodeSCFG &m_node;
};

  class DottedRuleInMemoryMBOT : public DottedRuleMBOT, public DottedRuleInMemory
{
 public:
  // used only to init dot stack.
  explicit DottedRuleInMemoryMBOT(const PhraseDictionaryNodeMBOT &node)
    :   DottedRuleMBOT()
     , DottedRuleInMemory(node)
      , m_mbotNode(node) {//std::cout<<"new DottedRuleInMemoryMBOT() " << this << m_mbotNode << std::endl;
      }

  DottedRuleInMemoryMBOT(const PhraseDictionaryNodeMBOT &node,
                     const ChartCellLabelMBOT &cellLabel,
                     const DottedRuleInMemoryMBOT &prev)
      :
  DottedRuleMBOT(cellLabel, prev)
          , DottedRuleInMemory(node, cellLabel, prev)

    , m_mbotNode(node) {//std::cout<<"new DottedRuleInMemoryMBOT(3) " << this << m_mbotNode
    //<< &m_NODE
    //<< std::endl;
  }

  const PhraseDictionaryNodeMBOT &GetLastNode() const {
    //FB : COMMENT IN AGAIN
    //std::cout << "Last Node returned from MBOT" << &(DottedRuleInMemoryMBOT::m_NODE) << std::endl;
      return m_mbotNode;}

 private:
  const PhraseDictionaryNodeMBOT &m_mbotNode;
};

typedef std::vector<const DottedRuleInMemory*> DottedRuleList;

// Collection of all in-memory DottedRules that share a common start point,
// grouped by end point.  Additionally, maintains a list of all
// DottedRules that could be expanded further, i.e. for which the
// corresponding PhraseDictionaryNodeSCFG is not a leaf.
class DottedRuleColl
{
protected:
  typedef std::vector<DottedRuleList> CollType;
  CollType m_coll;
  DottedRuleList m_expandableDottedRuleList;

public:
  typedef CollType::iterator iterator;
  typedef CollType::const_iterator const_iterator;

  const_iterator begin() const {
    return m_coll.begin();
  }
  const_iterator end() const {
    return m_coll.end();
  }
  iterator begin() {
    return m_coll.begin();
  }
  iterator end() {
    return m_coll.end();
  }

  DottedRuleColl(size_t size)
    : m_coll(size)
  {
      //std::cout<<"new DottedRuleColl() " << this << " size" << size << std::endl;
      }

  ~DottedRuleColl();

  const DottedRuleList &Get(size_t pos) const {
    return m_coll[pos];
  }

  DottedRuleList &Get(size_t pos) {
    return m_coll[pos];
  }

  virtual void Add(size_t pos, const DottedRuleInMemory *dottedRule) {
    CHECK(dottedRule);
    m_coll[pos].push_back(dottedRule);
    if (!dottedRule->GetLastNode().IsLeaf()) {
      m_expandableDottedRuleList.push_back(dottedRule);
    }
  }

  void Clear(size_t pos) {
#ifdef USE_BOOST_POOL
    m_coll[pos].clear();
#endif
  }

   const DottedRuleList &GetExpandableDottedRuleList() const {
    return m_expandableDottedRuleList;
  }

};

typedef std::vector<const DottedRuleInMemoryMBOT*> DottedRuleListMBOT;

// Collection of all in-memory DottedRules that share a common start point,
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
    :
  // DottedRuleColl(size),
m_mbotColl(size)
  {
      //std::cout<<"new DottedCollMBOT() : " << this << " size" << size << std::endl;
      }

  ~DottedRuleCollMBOT();

  const DottedRuleListMBOT &GetMBOT(size_t pos) const {
   //std::cout << "Getting MBOT list " << m_mbotColl.size() << "from DottedCollMBOT:" << this << " m_mbotColl[pos] " << &(m_mbotColl[pos]) << " pos " << pos << std::endl;
    return m_mbotColl[pos];
  }
  DottedRuleListMBOT &GetMBOT(size_t pos) {
    //std::cout << "Getting MBOT list " << m_mbotColl.size() << "from DottedCollMBOT:" << this << " m_mbotColl[pos] " << &(m_mbotColl[pos]) << " pos " << pos << std::endl;
    return m_mbotColl[pos];
  }

//FB : BEWARE : for testing
 size_t GetSizeMBOT(){
 return m_mbotColl.size();
 }

  void Add(size_t pos, const DottedRuleInMemoryMBOT *dottedRule) {
    //std::cout << "DCIM : ADDING DOTTED RULE : " << m_mbotExpandableDottedRuleList.size() << std::endl;
    CHECK(dottedRule);
    //std::cout << "DCIM : Added dotted rule : " << (*dottedRule) << std::endl;
    const PhraseDictionaryNodeMBOT &node = dottedRule->GetLastNode();
    //std::cout << "DCIM : Node PhraseDictionaryNodeMBOT " << &node << std::endl;
    const TargetPhraseCollection * tpc = node.GetTargetPhraseCollection();
    if(tpc != NULL)
    {
        //std::cout << "DCIM : TARGET NOT EMPTY : " << std::endl;
        TargetPhrase * tp = *((*tpc).begin());
        //cast to target phrase MBOT
        TargetPhraseMBOT * tpmbot = static_cast<TargetPhraseMBOT*>(tp);
        //std::cout << "DCIM : Target Phrase : Address " << &(tpmbot) << *tpmbot << std::endl;
    }
    else{//std::cout << "DCIM : TARGET PHRASE COLLECTION EMPTY" << std::endl;
    }
    //std::cout << "Added dotted rule adress: " << dottedRule << std::endl;
    //std::cout << "Added dotted rule TO DottedCollMBOT: " << this << std::endl;
    //std::cout << "DCIM : Push back dotted rule at position " << pos << std::endl;
    //std::cout << "m_mbotColl " << &(m_mbotColl[pos]) << std::endl;
    m_mbotColl[pos].push_back(dottedRule);
    //std::cout << "Size of coll at position " << pos << " : "<< m_mbotColl[pos].size() << std::endl;
    if (!dottedRule->GetLastNode().IsLeafMBOT()) {
      m_mbotExpandableDottedRuleList.push_back(dottedRule);
    }
    //std::cout << "DCIM : Adding dotted rule" << m_mbotExpandableDottedRuleList.size() << std::endl;
    //std::cout << "completion: Adding dotted rule adress: " << dottedRule << " " <<  m_mbotColl[pos][m_mbotColl[pos].size()-1] << std::endl;
  }

  void Clear(size_t pos) {
#ifdef USE_BOOST_POOL
    m_mbotColl[pos].clear();
#endif
  }

  const DottedRuleListMBOT &GetExpandableDottedRuleListMBOT() const {
    return m_mbotExpandableDottedRuleList;
  }

};


}
