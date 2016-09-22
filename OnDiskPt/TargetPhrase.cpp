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
#include "TargetPhrase.h"
#include "OnDiskWrapper.h"
#include "util/exception.hh"

#include <boost/algorithm/string.hpp>

using namespace std;

namespace OnDiskPt
{

TargetPhrase::TargetPhrase(size_t numScores)
  :m_scores(numScores)
{
}

TargetPhrase::TargetPhrase(const 	TargetPhrase &copy)
  :Phrase(copy)
  ,m_scores(copy.m_scores)
{

}

TargetPhrase::~TargetPhrase()
{
}

void TargetPhrase::SetLHS(WordPtr lhs)
{
  AddWord(lhs);
}

void TargetPhrase::Create1AlignFromString(const std::string &align1Str)
{
  vector<size_t> alignPoints;
  Moses::Tokenize<size_t>(alignPoints, align1Str, "-");
  UTIL_THROW_IF2(alignPoints.size() != 2, "Incorrectly formatted word alignment: " << align1Str);
  m_align.push_back(pair<size_t, size_t>(alignPoints[0], alignPoints[1]) );
}

void TargetPhrase::CreateAlignFromString(const std::string &alignStr)
{
  vector<std::string> alignPairs;
  boost::split(alignPairs, alignStr, boost::is_any_of("\t "));
  for (size_t i = 0; i < alignPairs.size(); ++i) {
    vector<size_t> alignPoints;
    Moses::Tokenize<size_t>(alignPoints, alignPairs[i], "-");
    m_align.push_back(pair<size_t, size_t>(alignPoints[0], alignPoints[1]) );
  }
}


void TargetPhrase::SetScore(float score, size_t ind)
{
  assert(ind < m_scores.size());
  m_scores[ind] = score;
}

class AlignOrderer
{
public:
  bool operator()(const AlignPair &a, const AlignPair &b) const {
    return a.first < b.first;
  }
};

void TargetPhrase::SortAlign()
{
  std::sort(m_align.begin(), m_align.end(), AlignOrderer());
}

char *TargetPhrase::WriteToMemory(OnDiskWrapper &onDiskWrapper, size_t &memUsed) const
{
  size_t phraseSize = GetSize();
  size_t targetWordSize = onDiskWrapper.GetTargetWordSize();

  const PhrasePtr sp = GetSourcePhrase();
  size_t spSize = sp->GetSize();
  size_t sourceWordSize = onDiskWrapper.GetSourceWordSize();

  size_t memNeeded = sizeof(uint64_t)						// num of words
                     + targetWordSize * phraseSize	// actual words. lhs as last words
                     + sizeof(uint64_t)					// num source words
                     + sourceWordSize * spSize;   // actual source words

  memUsed = 0;
  uint64_t *mem = (uint64_t*) malloc(memNeeded);

  // write size
  mem[0] = phraseSize;
  memUsed += sizeof(uint64_t);

  // write each word
  for (size_t pos = 0; pos < phraseSize; ++pos) {
    const Word &word = GetWord(pos);
    char *currPtr = (char*)mem + memUsed;
    memUsed += word.WriteToMemory((char*) currPtr);
  }

  // write size of source phrase and all source words
  char *currPtr = (char*)mem + memUsed;
  uint64_t *memTmp = (uint64_t*) currPtr;
  memTmp[0] = spSize;
  memUsed += sizeof(uint64_t);
  for (size_t pos = 0; pos < spSize; ++pos) {
    const Word &word = sp->GetWord(pos);
    char *currPtr = (char*)mem + memUsed;
    memUsed += word.WriteToMemory((char*) currPtr);
  }

  assert(memUsed == memNeeded);
  return (char *) mem;
}

void TargetPhrase::Save(OnDiskWrapper &onDiskWrapper)
{
  // save in target ind
  size_t memUsed;
  char *mem = WriteToMemory(onDiskWrapper, memUsed);

  std::fstream &file = onDiskWrapper.GetFileTargetInd();

  uint64_t startPos = file.tellp();

  file.seekp(0, ios::end);
  file.write(mem, memUsed);

#ifndef NDEBUG
  uint64_t endPos = file.tellp();
  assert(startPos + memUsed == endPos);
#endif

  m_filePos = startPos;
  free(mem);
}

char *TargetPhrase::WriteOtherInfoToMemory(OnDiskWrapper &onDiskWrapper, size_t &memUsed) const
{
  // allocate mem
  size_t numScores = onDiskWrapper.GetNumScores()
                     ,numAlign = GetAlign().size();
  size_t sparseFeatureSize = m_sparseFeatures.size();
  size_t propSize = m_property.size();

  size_t memNeeded = sizeof(uint64_t) // file pos (phrase id)
                     + sizeof(uint64_t) + 2 * sizeof(uint64_t) * numAlign // align
                     + sizeof(float) * numScores // scores
                     + sizeof(uint64_t) + sparseFeatureSize // sparse features string
                     + sizeof(uint64_t) + propSize; // property string

  char *mem = (char*) malloc(memNeeded);
  //memset(mem, 0, memNeeded);

  memUsed = 0;

  // phrase id
  memcpy(mem, &m_filePos, sizeof(uint64_t));
  memUsed += sizeof(uint64_t);

  // align
  size_t tmp = WriteAlignToMemory(mem + memUsed);
  memUsed += tmp;

  // scores
  memUsed += WriteScoresToMemory(mem + memUsed);

  // sparse features
  memUsed += WriteStringToMemory(mem + memUsed, m_sparseFeatures);

  // property string
  memUsed += WriteStringToMemory(mem + memUsed, m_property);

  //DebugMem(mem, memNeeded);
  assert(memNeeded == memUsed);
  return mem;
}

size_t TargetPhrase::WriteStringToMemory(char *mem, const std::string &str) const
{
  size_t memUsed = 0;
  uint64_t *memTmp = (uint64_t*) mem;

  size_t strSize = str.size();
  memTmp[0] = strSize;
  memUsed += sizeof(uint64_t);

  const char *charStr = str.c_str();
  memcpy(mem + memUsed, charStr, strSize);
  memUsed += strSize;

  return memUsed;
}

size_t TargetPhrase::WriteAlignToMemory(char *mem) const
{
  size_t memUsed = 0;

  // num of alignments
  uint64_t numAlign = m_align.size();
  memcpy(mem, &numAlign, sizeof(numAlign));
  memUsed += sizeof(numAlign);

  // actual alignments
  AlignType::const_iterator iter;
  for (iter = m_align.begin(); iter != m_align.end(); ++iter) {
    const AlignPair &alignPair = *iter;

    memcpy(mem + memUsed, &alignPair.first, sizeof(alignPair.first));
    memUsed += sizeof(alignPair.first);

    memcpy(mem + memUsed, &alignPair.second, sizeof(alignPair.second));
    memUsed += sizeof(alignPair.second);
  }

  return memUsed;
}

size_t TargetPhrase::WriteScoresToMemory(char *mem) const
{
  float *scoreMem = (float*) mem;

  for (size_t ind = 0; ind < m_scores.size(); ++ind)
    scoreMem[ind] = m_scores[ind];

  size_t memUsed = sizeof(float) * m_scores.size();
  return memUsed;
}

uint64_t TargetPhrase::ReadOtherInfoFromFile(uint64_t filePos, std::fstream &fileTPColl)
{
  assert(filePos == (uint64_t)fileTPColl.tellg());

  uint64_t memUsed = 0;
  fileTPColl.read((char*) &m_filePos, sizeof(uint64_t));
  memUsed += sizeof(uint64_t);
  assert(m_filePos != 0);

  memUsed += ReadAlignFromFile(fileTPColl);
  assert((memUsed + filePos) == (uint64_t)fileTPColl.tellg());

  memUsed += ReadScoresFromFile(fileTPColl);
  assert((memUsed + filePos) == (uint64_t)fileTPColl.tellg());

  // sparse features
  memUsed += ReadStringFromFile(fileTPColl, m_sparseFeatures);

  // properties
  memUsed += ReadStringFromFile(fileTPColl, m_property);

  return memUsed;
}

uint64_t TargetPhrase::ReadStringFromFile(std::fstream &fileTPColl, std::string &outStr)
{
  uint64_t bytesRead = 0;

  uint64_t strSize;
  fileTPColl.read((char*) &strSize, sizeof(uint64_t));
  bytesRead += sizeof(uint64_t);

  if (strSize) {
    char *mem = (char*) malloc(strSize + 1);
    mem[strSize] = '\0';
    fileTPColl.read(mem, strSize);
    outStr = string(mem);
    free(mem);

    bytesRead += strSize;
  }

  return bytesRead;
}

uint64_t TargetPhrase::ReadFromFile(std::fstream &fileTP)
{
  uint64_t bytesRead = 0;

  fileTP.seekg(m_filePos);

  uint64_t numWords;
  fileTP.read((char*) &numWords, sizeof(uint64_t));
  bytesRead += sizeof(uint64_t);

  for (size_t ind = 0; ind < numWords; ++ind) {
    WordPtr word(new Word());
    bytesRead += word->ReadFromFile(fileTP);
    AddWord(word);
  }

  // read source words
  uint64_t numSourceWords;
  fileTP.read((char*) &numSourceWords, sizeof(uint64_t));
  bytesRead += sizeof(uint64_t);

  PhrasePtr sp(new SourcePhrase());
  for (size_t ind = 0; ind < numSourceWords; ++ind) {
    WordPtr word( new Word());
    bytesRead += word->ReadFromFile(fileTP);
    sp->AddWord(word);
  }
  SetSourcePhrase(sp);

  return bytesRead;
}

uint64_t TargetPhrase::ReadAlignFromFile(std::fstream &fileTPColl)
{
  uint64_t bytesRead = 0;

  uint64_t numAlign;
  fileTPColl.read((char*) &numAlign, sizeof(uint64_t));
  bytesRead += sizeof(uint64_t);

  for (size_t ind = 0; ind < numAlign; ++ind) {
    AlignPair alignPair;
    fileTPColl.read((char*) &alignPair.first, sizeof(uint64_t));
    fileTPColl.read((char*) &alignPair.second, sizeof(uint64_t));
    m_align.push_back(alignPair);

    bytesRead += sizeof(uint64_t) * 2;
  }

  return bytesRead;
}

uint64_t TargetPhrase::ReadScoresFromFile(std::fstream &fileTPColl)
{
  UTIL_THROW_IF2(m_scores.size() == 0, "Translation rules must must have some scores");

  uint64_t bytesRead = 0;

  for (size_t ind = 0; ind < m_scores.size(); ++ind) {
    fileTPColl.read((char*) &m_scores[ind], sizeof(float));

    bytesRead += sizeof(float);
  }

  std::transform(m_scores.begin(),m_scores.end(),m_scores.begin(), Moses::TransformScore);
  std::transform(m_scores.begin(),m_scores.end(),m_scores.begin(), Moses::FloorScore);

  return bytesRead;
}

void TargetPhrase::DebugPrint(ostream &out, const Vocab &vocab) const
{
  Phrase::DebugPrint(out, vocab);

  for (size_t ind = 0; ind < m_align.size(); ++ind) {
    const AlignPair &alignPair = m_align[ind];
    out << alignPair.first << "-" << alignPair.second << " ";
  }
  out << ", ";

  for (size_t ind = 0; ind < m_scores.size(); ++ind) {
    out << m_scores[ind] << " ";
  }

  return;
}

std::ostream& operator<<(std::ostream &out, const TargetPhrase &phrase)
{
  out << (const Phrase&) phrase << ", " ;

  for (size_t ind = 0; ind < phrase.m_align.size(); ++ind) {
    const AlignPair &alignPair = phrase.m_align[ind];
    out << alignPair.first << "-" << alignPair.second << " ";
  }
  out << ", ";

  for (size_t ind = 0; ind < phrase.m_scores.size(); ++ind) {
    out << phrase.m_scores[ind] << " ";
  }

  return out;
}

} // namespace

