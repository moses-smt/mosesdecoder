// $Id$
/***********************************************************************
 Moses - factored phrase-based, hierarchical and syntactic language decoder
 Copyright (C) 2009 Hieu Hoang

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

#include "TargetPhrase.h"
#include "Vocab.h"
#include <boost/shared_ptr.hpp>

namespace Moses
{
class TargetPhraseCollection;
class PhraseDictionary;
}

namespace OnDiskPt
{

/** A vector of target phrases
 */
class TargetPhraseCollection
{
  class TargetPhraseOrderByScore
  {
  public:
    bool operator()(const TargetPhrase* a, const TargetPhrase *b) const {
      return a->GetScore(s_sortScoreInd) > b->GetScore(s_sortScoreInd);
    }
  };

protected:
  typedef std::vector<TargetPhrase*> CollType;
  CollType m_coll;
  uint64_t m_filePos;
  std::string m_debugStr;

public:
  typedef boost::shared_ptr<TargetPhraseCollection const> shared_const_ptr;
  typedef boost::shared_ptr<TargetPhraseCollection> shared_ptr;

  static size_t s_sortScoreInd;

  TargetPhraseCollection();
  TargetPhraseCollection(const TargetPhraseCollection &copy);

  ~TargetPhraseCollection();
  void AddTargetPhrase(TargetPhrase *targetPhrase);
  void Sort(size_t tableLimit);

  void Save(OnDiskWrapper &onDiskWrapper);

  size_t GetSize() const {
    return m_coll.size();
  }

  const TargetPhrase &GetTargetPhrase(size_t ind) const;

  uint64_t GetFilePos() const;

  void ReadFromFile(size_t tableLimit, uint64_t filePos, OnDiskWrapper &onDiskWrapper);

  const std::string GetDebugStr() const;
  void SetDebugStr(const std::string &str);

};

}

