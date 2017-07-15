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

#include <algorithm>
#include <iostream>
#include "moses/Util.h"
#include "TargetPhraseCollection.h"
#include "Vocab.h"
#include "OnDiskWrapper.h"

using namespace std;

namespace OnDiskPt
{

size_t TargetPhraseCollection::s_sortScoreInd;

TargetPhraseCollection::TargetPhraseCollection()
  :m_filePos(777)
{}

TargetPhraseCollection::TargetPhraseCollection(const TargetPhraseCollection &copy)
  :m_filePos(copy.m_filePos)
  ,m_debugStr(copy.m_debugStr)
{
}

TargetPhraseCollection::~TargetPhraseCollection()
{
  Moses::RemoveAllInColl(m_coll);
}

void TargetPhraseCollection::AddTargetPhrase(TargetPhrase *targetPhrase)
{
  m_coll.push_back(targetPhrase);
}

void TargetPhraseCollection::Sort(size_t tableLimit)
{
  std::sort(m_coll.begin(), m_coll.end(), TargetPhraseOrderByScore());

  if (tableLimit && m_coll.size() > tableLimit) {
    CollType::iterator iter;
    for (iter = m_coll.begin() + tableLimit ; iter != m_coll.end(); ++iter) {
      delete *iter;
    }
    m_coll.resize(tableLimit);
  }
}

void TargetPhraseCollection::Save(OnDiskWrapper &onDiskWrapper)
{
  std::fstream &file = onDiskWrapper.GetFileTargetColl();

  size_t memUsed = sizeof(uint64_t);
  char *mem = (char*) malloc(memUsed);

  // size of coll
  uint64_t numPhrases = GetSize();
  ((uint64_t*)mem)[0] = numPhrases;

  // MAIN LOOP
  CollType::iterator iter;
  for (iter = m_coll.begin(); iter != m_coll.end(); ++iter) {
    // save phrase
    TargetPhrase &targetPhrase = **iter;
    targetPhrase.Save(onDiskWrapper);

    // save coll
    size_t memUsedTPOtherInfo;
    char *memTPOtherInfo = targetPhrase.WriteOtherInfoToMemory(onDiskWrapper, memUsedTPOtherInfo);

    // expand existing mem
    mem = (char*) realloc(mem, memUsed + memUsedTPOtherInfo);
    memcpy(mem + memUsed, memTPOtherInfo, memUsedTPOtherInfo);
    memUsed += memUsedTPOtherInfo;

    free(memTPOtherInfo);
  }

  // total number of bytes
  //((uint64_t*)mem)[0] = (uint64_t) memUsed;

  uint64_t startPos = file.tellp();
  file.seekp(0, ios::end);
  file.write((char*) mem, memUsed);

  free(mem);

#ifndef NDEBUG
  uint64_t endPos = file.tellp();
  assert(startPos + memUsed == endPos);
#endif
  m_filePos = startPos;

}

void TargetPhraseCollection::ReadFromFile(size_t tableLimit, uint64_t filePos, OnDiskWrapper &onDiskWrapper)
{
  fstream &fileTPColl = onDiskWrapper.GetFileTargetColl();
  fstream &fileTP = onDiskWrapper.GetFileTargetInd();

  size_t numScores = onDiskWrapper.GetNumScores();


  uint64_t numPhrases;

  uint64_t currFilePos = filePos;
  fileTPColl.seekg(filePos);
  fileTPColl.read((char*) &numPhrases, sizeof(uint64_t));

  // table limit
  if (tableLimit) {
    numPhrases = std::min(numPhrases, (uint64_t) tableLimit);
  }

  currFilePos += sizeof(uint64_t);

  for (size_t ind = 0; ind < numPhrases; ++ind) {
    TargetPhrase *tp = new TargetPhrase(numScores);

    uint64_t sizeOtherInfo = tp->ReadOtherInfoFromFile(currFilePos, fileTPColl);
    tp->ReadFromFile(fileTP);

    currFilePos += sizeOtherInfo;

    m_coll.push_back(tp);
  }
}

uint64_t TargetPhraseCollection::GetFilePos() const
{
  return m_filePos;
}

const std::string TargetPhraseCollection::GetDebugStr() const
{
  return m_debugStr;
}

void TargetPhraseCollection::SetDebugStr(const std::string &str)
{
  m_debugStr = str;
}

const TargetPhrase &TargetPhraseCollection::GetTargetPhrase(size_t ind) const
{
  assert(ind < GetSize());
  return *m_coll[ind];
}

}


