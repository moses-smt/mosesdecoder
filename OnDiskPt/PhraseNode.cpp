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
#include "PhraseNode.h"
#include "OnDiskWrapper.h"
#include "TargetPhraseCollection.h"
#include "SourcePhrase.h"
#include "moses/Util.h"
#include "util/exception.hh"

using namespace std;

namespace OnDiskPt
{

size_t PhraseNode::GetNodeSize(size_t numChildren, size_t wordSize, size_t countSize)
{
  size_t ret = sizeof(uint64_t) * 2 // num children, value
               + (wordSize + sizeof(uint64_t)) * numChildren // word + ptr to next source node
               + sizeof(float) * countSize; // count info
  return ret;
}

PhraseNode::PhraseNode()
  : m_value(0)
  ,m_currChild(NULL)
  ,m_saved(false)
  ,m_memLoad(NULL)
{
}

PhraseNode::PhraseNode(uint64_t filePos, OnDiskWrapper &onDiskWrapper)
  :m_counts(onDiskWrapper.GetNumCounts())
{
  // load saved node
  m_filePos = filePos;

  size_t countSize = onDiskWrapper.GetNumCounts();

  std::fstream &file = onDiskWrapper.GetFileSource();
  file.seekg(filePos);
  assert(filePos == (uint64_t)file.tellg());

  file.read((char*) &m_numChildrenLoad, sizeof(uint64_t));

  size_t memAlloc = GetNodeSize(m_numChildrenLoad, onDiskWrapper.GetSourceWordSize(), countSize);
  m_memLoad = (char*) malloc(memAlloc);

  // go to start of node again
  file.seekg(filePos);
  assert(filePos == (uint64_t)file.tellg());

  // read everything into memory
  file.read(m_memLoad, memAlloc);
  assert(filePos + memAlloc == (uint64_t)file.tellg());

  // get value
  m_value = ((uint64_t*)m_memLoad)[1];

  // get counts
  float *memFloat = (float*) (m_memLoad + sizeof(uint64_t) * 2);

  assert(countSize == 1);
  m_counts[0] = memFloat[0];

  m_memLoadLast = m_memLoad + memAlloc;
}

PhraseNode::~PhraseNode()
{
  free(m_memLoad);
}

float PhraseNode::GetCount(size_t ind) const
{
  return m_counts[ind];
}

void PhraseNode::Save(OnDiskWrapper &onDiskWrapper, size_t pos, size_t tableLimit)
{
  UTIL_THROW_IF2(m_saved, "Already saved");

  // save this node
  m_targetPhraseColl.Sort(tableLimit);
  m_targetPhraseColl.Save(onDiskWrapper);
  m_value = m_targetPhraseColl.GetFilePos();

  size_t numCounts = onDiskWrapper.GetNumCounts();

  size_t memAlloc = GetNodeSize(GetSize(), onDiskWrapper.GetSourceWordSize(), numCounts);
  char *mem = (char*) malloc(memAlloc);
  //memset(mem, 0xfe, memAlloc);

  size_t memUsed = 0;
  uint64_t *memArray = (uint64_t*) mem;
  memArray[0] = GetSize(); // num of children
  memArray[1] = m_value;   // file pos of corresponding target phrases
  memUsed += 2 * sizeof(uint64_t);

  // count info
  float *memFloat = (float*) (mem + memUsed);
  UTIL_THROW_IF2(numCounts != 1, "Can only store 1 phrase count");
  memFloat[0] = (m_counts.size() == 0) ? DEFAULT_COUNT : m_counts[0]; // if count = 0, put in very large num to make sure its still used. HACK
  memUsed += sizeof(float) * numCounts;

  // recursively save chm_countsildren
  ChildColl::iterator iter;
  for (iter = m_children.begin(); iter != m_children.end(); ++iter) {
    const Word &childWord = iter->first;
    PhraseNode &childNode = iter->second;

    // recursive
    if (!childNode.Saved())
      childNode.Save(onDiskWrapper, pos + 1, tableLimit);

    char *currMem = mem + memUsed;
    size_t wordMemUsed = childWord.WriteToMemory(currMem);
    memUsed += wordMemUsed;

    uint64_t *memArray = (uint64_t*) (mem + memUsed);
    memArray[0] = childNode.GetFilePos();
    memUsed += sizeof(uint64_t);

  }

  // save this node
  //Moses::DebugMem(mem, memAlloc);
  assert(memUsed == memAlloc);

  std::fstream &file = onDiskWrapper.GetFileSource();
  m_filePos = file.tellp();
  file.seekp(0, ios::end);
  file.write(mem, memUsed);

  uint64_t endPos = file.tellp();
  assert(m_filePos + memUsed == endPos);

  free(mem);

  m_children.clear();
  m_saved = true;
}

void PhraseNode::AddTargetPhrase(const SourcePhrase &sourcePhrase, TargetPhrase *targetPhrase
                                 , OnDiskWrapper &onDiskWrapper, size_t tableLimit
                                 , const std::vector<float> &counts, OnDiskPt::PhrasePtr spShort)
{
  AddTargetPhrase(0, sourcePhrase, targetPhrase, onDiskWrapper, tableLimit, counts, spShort);
}

void PhraseNode::AddTargetPhrase(size_t pos, const SourcePhrase &sourcePhrase
                                 , TargetPhrase *targetPhrase, OnDiskWrapper &onDiskWrapper
                                 , size_t tableLimit, const std::vector<float> &counts, OnDiskPt::PhrasePtr spShort)
{
  size_t phraseSize = sourcePhrase.GetSize();
  if (pos < phraseSize) {
    const Word &word = sourcePhrase.GetWord(pos);

    PhraseNode &node = m_children[word];
    if (m_currChild != &node) {
      // new node
      node.SetPos(pos);

      if (m_currChild) {
        m_currChild->Save(onDiskWrapper, pos, tableLimit);
      }

      m_currChild = &node;
    }

    // keep searching for target phrase node..
    node.AddTargetPhrase(pos + 1, sourcePhrase, targetPhrase, onDiskWrapper, tableLimit, counts, spShort);
  } else {
    // drilled down to the right node
    m_counts = counts;
    targetPhrase->SetSourcePhrase(spShort);
    m_targetPhraseColl.AddTargetPhrase(targetPhrase);
  }
}

const PhraseNode *PhraseNode::GetChild(const Word &wordSought, OnDiskWrapper &onDiskWrapper) const
{
  const PhraseNode *ret = NULL;

  int l = 0;
  int r = m_numChildrenLoad - 1;
  int x;

  while (r >= l) {
    x = (l + r) / 2;

    Word wordFound;
    uint64_t childFilePos;
    GetChild(wordFound, childFilePos, x, onDiskWrapper);

    if (wordSought == wordFound) {
      ret = new PhraseNode(childFilePos, onDiskWrapper);
      break;
    }
    if (wordSought < wordFound)
      r = x - 1;
    else
      l = x + 1;
  }

  return ret;
}

void PhraseNode::GetChild(Word &wordFound, uint64_t &childFilePos, size_t ind, OnDiskWrapper &onDiskWrapper) const
{

  size_t wordSize = onDiskWrapper.GetSourceWordSize();
  size_t childSize = wordSize + sizeof(uint64_t);

  char *currMem = m_memLoad
                  + sizeof(uint64_t) * 2 // size & file pos of target phrase coll
                  + sizeof(float) * onDiskWrapper.GetNumCounts() // count info
                  + childSize * ind;

  size_t memRead = ReadChild(wordFound, childFilePos, currMem);
  assert(memRead == childSize);
}

size_t PhraseNode::ReadChild(Word &wordFound, uint64_t &childFilePos, const char *mem) const
{
  size_t memRead = wordFound.ReadFromMemory(mem);

  const char *currMem = mem + memRead;
  uint64_t *memArray = (uint64_t*) (currMem);
  childFilePos = memArray[0];

  memRead += sizeof(uint64_t);
  return memRead;
}

TargetPhraseCollection::shared_ptr
PhraseNode::
GetTargetPhraseCollection(size_t tableLimit, OnDiskWrapper &onDiskWrapper) const
{
  TargetPhraseCollection::shared_ptr ret(new TargetPhraseCollection);
  if (m_value > 0) ret->ReadFromFile(tableLimit, m_value, onDiskWrapper);
  return ret;
}

std::ostream& operator<<(std::ostream &out, const PhraseNode &node)
{
  out << "node (" << node.GetFilePos() << "," << node.GetValue() << "," << node.m_pos << ")";
  return out;
}

}

