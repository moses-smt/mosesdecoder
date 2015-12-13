#pragma once
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
#include <fstream>
#include <vector>
#include <map>
#include "Word.h"
#include "TargetPhraseCollection.h"
#include "Phrase.h"

namespace OnDiskPt
{

class OnDiskWrapper;
class SourcePhrase;

/** A node in the source tree trie */
class PhraseNode
{
  friend std::ostream& operator<<(std::ostream&, const PhraseNode&);
protected:
  uint64_t m_filePos, m_value;

  typedef std::map<Word, PhraseNode> ChildColl;
  ChildColl m_children;
  PhraseNode *m_currChild;
  bool m_saved;
  size_t m_pos;
  std::vector<float> m_counts;

  TargetPhraseCollection m_targetPhraseColl;

  char *m_memLoad, *m_memLoadLast;
  uint64_t m_numChildrenLoad;

  void AddTargetPhrase(size_t pos, const SourcePhrase &sourcePhrase
                       , TargetPhrase *targetPhrase, OnDiskWrapper &onDiskWrapper
                       , size_t tableLimit, const std::vector<float> &counts, OnDiskPt::PhrasePtr spShort);
  size_t ReadChild(Word &wordFound, uint64_t &childFilePos, const char *mem) const;
  void GetChild(Word &wordFound, uint64_t &childFilePos, size_t ind, OnDiskWrapper &onDiskWrapper) const;

public:
  static size_t GetNodeSize(size_t numChildren, size_t wordSize, size_t countSize);

  PhraseNode(); // unsaved node
  PhraseNode(uint64_t filePos, OnDiskWrapper &onDiskWrapper); // load saved node
  ~PhraseNode();

  void Add(const Word &word, uint64_t nextFilePos, size_t wordSize);
  void Save(OnDiskWrapper &onDiskWrapper, size_t pos, size_t tableLimit);

  void AddTargetPhrase(const SourcePhrase &sourcePhrase, TargetPhrase *targetPhrase
                       , OnDiskWrapper &onDiskWrapper, size_t tableLimit
                       , const std::vector<float> &counts, OnDiskPt::PhrasePtr spShort);

  uint64_t GetFilePos() const {
    return m_filePos;
  }
  uint64_t GetValue() const {
    return m_value;
  }
  void SetValue(uint64_t value) {
    m_value = value;
  }
  size_t GetSize() const {
    return m_children.size();
  }

  bool Saved() const {
    return m_saved;
  }

  void SetPos(size_t pos) {
    m_pos = pos;
  }

  const PhraseNode *GetChild(const Word &wordSought, OnDiskWrapper &onDiskWrapper) const;

  TargetPhraseCollection::shared_ptr
  GetTargetPhraseCollection(size_t tableLimit,
                            OnDiskWrapper &onDiskWrapper) const;

  void AddCounts(const std::vector<float> &counts) {
    m_counts = counts;
  }
  float GetCount(size_t ind) const;

};

}

