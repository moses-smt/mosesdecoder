// $Id$
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang

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

#include <vector>
#include "DotChart.h"

namespace OnDiskPt
{
class PhraseNode;
}

namespace Moses
{

/** @todo what is this?
 */
class DottedRuleOnDisk : public DottedRule
{
public:
  // used only to init dot stack.
  explicit DottedRuleOnDisk(const OnDiskPt::PhraseNode &lastNode)
    : DottedRule()
    , m_lastNode(lastNode)
    , m_done(false) {}

  DottedRuleOnDisk(const OnDiskPt::PhraseNode &lastNode,
                   const ChartCellLabel &cellLabel,
                   const DottedRuleOnDisk &prev)
    : DottedRule(cellLabel, prev)
    , m_lastNode(lastNode)
    , m_done(false) {}

  const OnDiskPt::PhraseNode &GetLastNode() const {
    return m_lastNode;
  }

  bool Done() const {
    return m_done;
  }
  void Done(bool value) const {
    m_done = value;
  }

private:
  const OnDiskPt::PhraseNode &m_lastNode;
  mutable bool m_done;
};

class DottedRuleCollOnDisk
{
protected:
  typedef std::vector<const DottedRuleOnDisk*> CollType;
  CollType m_coll;

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

  const DottedRuleOnDisk &Get(size_t ind) const {
    return *m_coll[ind];
  }

  void Add(const DottedRuleOnDisk *dottedRule) {
    m_coll.push_back(dottedRule);
  }
  void Delete(size_t ind) {
    //delete m_coll[ind];
    m_coll.erase(m_coll.begin() + ind);
  }

  size_t GetSize() const {
    return m_coll.size();
  }

};

class SavedNodeOnDisk
{
  const DottedRuleOnDisk *m_dottedRule;

public:
  SavedNodeOnDisk(const DottedRuleOnDisk *dottedRule)
    :m_dottedRule(dottedRule) {
    UTIL_THROW_IF2(m_dottedRule == NULL, "Dotted rule is null");
  }

  ~SavedNodeOnDisk() {
    delete m_dottedRule;
  }

  const DottedRuleOnDisk &GetDottedRule() const {
    return *m_dottedRule;
  }
};

class DottedRuleStackOnDisk
{
  // coll of coll of processed rules
public:
  typedef std::vector<SavedNodeOnDisk*> SavedNodeColl;

protected:
  typedef std::vector<DottedRuleCollOnDisk*> CollType;
  CollType m_coll;

  SavedNodeColl m_savedNode;

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

  DottedRuleStackOnDisk(size_t size);
  ~DottedRuleStackOnDisk();

  const DottedRuleCollOnDisk &Get(size_t pos) const {
    return *m_coll[pos];
  }
  DottedRuleCollOnDisk &Get(size_t pos) {
    return *m_coll[pos];
  }

  const DottedRuleCollOnDisk &back() const {
    return *m_coll.back();
  }

  void Add(size_t pos, const DottedRuleOnDisk *dottedRule) {
    UTIL_THROW_IF2(dottedRule == NULL, "Dotted rule is null");

    m_coll[pos]->Add(dottedRule);
    m_savedNode.push_back(new SavedNodeOnDisk(dottedRule));
  }

  const SavedNodeColl &GetSavedNodeColl() const {
    return m_savedNode;
  }

  void SortSavedNodes();

};

}

