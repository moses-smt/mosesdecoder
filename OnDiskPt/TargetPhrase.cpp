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
#include "moses/TargetPhrase.h"
#include "moses/TranslationModel/PhraseDictionary.h"
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

  size_t memNeeded = sizeof(UINT64)						// num of words
                     + targetWordSize * phraseSize	// actual words. lhs as last words
                     + sizeof(UINT64)					// num source words
                     + sourceWordSize * spSize;   // actual source words

  memUsed = 0;
  UINT64 *mem = (UINT64*) malloc(memNeeded);

  // write size
  mem[0] = phraseSize;
  memUsed += sizeof(UINT64);

  // write each word
  for (size_t pos = 0; pos < phraseSize; ++pos) {
    const Word &word = GetWord(pos);
    char *currPtr = (char*)mem + memUsed;
    memUsed += word.WriteToMemory((char*) currPtr);
  }

  // write size of source phrase and all source words
  char *currPtr = (char*)mem + memUsed;
  UINT64 *memTmp = (UINT64*) currPtr;
  memTmp[0] = spSize;
  memUsed += sizeof(UINT64);
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

  UINT64 startPos = file.tellp();

  file.seekp(0, ios::end);
  file.write(mem, memUsed);

  UINT64 endPos = file.tellp();
  assert(startPos + memUsed == endPos);

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

  size_t memNeeded = sizeof(UINT64) // file pos (phrase id)
                     + sizeof(UINT64) + 2 * sizeof(UINT64) * numAlign // align
                     + sizeof(float) * numScores // scores
                     + sizeof(UINT64) + sparseFeatureSize // sparse features string
                     + sizeof(UINT64) + propSize; // property string

  char *mem = (char*) malloc(memNeeded);
  //memset(mem, 0, memNeeded);

  memUsed = 0;

  // phrase id
  memcpy(mem, &m_filePos, sizeof(UINT64));
  memUsed += sizeof(UINT64);

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
  UINT64 *memTmp = (UINT64*) mem;

  size_t strSize = str.size();
  memTmp[0] = strSize;
  memUsed += sizeof(UINT64);

  const char *charStr = str.c_str();
  memcpy(mem + memUsed, charStr, strSize);
  memUsed += strSize;

  return memUsed;
}

size_t TargetPhrase::WriteAlignToMemory(char *mem) const
{
  size_t memUsed = 0;

  // num of alignments
  UINT64 numAlign = m_align.size();
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


Moses::TargetPhrase *TargetPhrase::ConvertToMoses(const std::vector<Moses::FactorType> & inputFactors
    , const std::vector<Moses::FactorType> &outputFactors
    , const Vocab &vocab
    , const Moses::PhraseDictionary &phraseDict
    , const std::vector<float> &weightT
    , bool isSyntax) const
{
  Moses::TargetPhrase *ret = new Moses::TargetPhrase(&phraseDict);

  // words
  size_t phraseSize = GetSize();
  UTIL_THROW_IF2(phraseSize == 0, "Target phrase cannot be empty"); // last word is lhs
  if (isSyntax) {
    --phraseSize;
  }

  for (size_t pos = 0; pos < phraseSize; ++pos) {
    GetWord(pos).ConvertToMoses(outputFactors, vocab, ret->AddWord());
  }

  // alignments
  int index = 0;
  Moses::AlignmentInfo::CollType alignTerm, alignNonTerm;
  std::set<std::pair<size_t, size_t> > alignmentInfo;
  const PhrasePtr sp = GetSourcePhrase();
  for (size_t ind = 0; ind < m_align.size(); ++ind) {
    const std::pair<size_t, size_t> &entry = m_align[ind];
    alignmentInfo.insert(entry);
    size_t sourcePos = entry.first;
    size_t targetPos = entry.second;

    if (GetWord(targetPos).IsNonTerminal()) {
      alignNonTerm.insert(std::pair<size_t,size_t>(sourcePos, targetPos));
    } else {
      alignTerm.insert(std::pair<size_t,size_t>(sourcePos, targetPos));
    }

  }
  ret->SetAlignTerm(alignTerm);
  ret->SetAlignNonTerm(alignNonTerm);

  if (isSyntax) {
    Moses::Word *lhsTarget = new Moses::Word(true);
    GetWord(GetSize() - 1).ConvertToMoses(outputFactors, vocab, *lhsTarget);
    ret->SetTargetLHS(lhsTarget);
  }

  // set source phrase
  Moses::Phrase mosesSP(Moses::Input);
  for (size_t pos = 0; pos < sp->GetSize(); ++pos) {
    sp->GetWord(pos).ConvertToMoses(inputFactors, vocab, mosesSP.AddWord());
  }

  // scores
  ret->GetScoreBreakdown().Assign(&phraseDict, m_scores);

  // sparse features
  ret->GetScoreBreakdown().Assign(&phraseDict, m_sparseFeatures);

  // property
  ret->SetProperties(m_property);

  ret->EvaluateInIsolation(mosesSP, phraseDict.GetFeaturesToApply());

  return ret;
}

UINT64 TargetPhrase::ReadOtherInfoFromFile(UINT64 filePos, std::fstream &fileTPColl)
{
  assert(filePos == (UINT64)fileTPColl.tellg());

  UINT64 memUsed = 0;
  fileTPColl.read((char*) &m_filePos, sizeof(UINT64));
  memUsed += sizeof(UINT64);
  assert(m_filePos != 0);

  memUsed += ReadAlignFromFile(fileTPColl);
  assert((memUsed + filePos) == (UINT64)fileTPColl.tellg());

  memUsed += ReadScoresFromFile(fileTPColl);
  assert((memUsed + filePos) == (UINT64)fileTPColl.tellg());

  // sparse features
  memUsed += ReadStringFromFile(fileTPColl, m_sparseFeatures);

  // properties
  memUsed += ReadStringFromFile(fileTPColl, m_property);

  return memUsed;
}

UINT64 TargetPhrase::ReadStringFromFile(std::fstream &fileTPColl, std::string &outStr)
{
  UINT64 bytesRead = 0;

  UINT64 strSize;
  fileTPColl.read((char*) &strSize, sizeof(UINT64));
  bytesRead += sizeof(UINT64);

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

UINT64 TargetPhrase::ReadFromFile(std::fstream &fileTP)
{
  UINT64 bytesRead = 0;

  fileTP.seekg(m_filePos);

  UINT64 numWords;
  fileTP.read((char*) &numWords, sizeof(UINT64));
  bytesRead += sizeof(UINT64);

  for (size_t ind = 0; ind < numWords; ++ind) {
    WordPtr word(new Word());
    bytesRead += word->ReadFromFile(fileTP);
    AddWord(word);
  }

  // read source words
  UINT64 numSourceWords;
  fileTP.read((char*) &numSourceWords, sizeof(UINT64));
  bytesRead += sizeof(UINT64);

  PhrasePtr sp(new SourcePhrase());
  for (size_t ind = 0; ind < numSourceWords; ++ind) {
    WordPtr word( new Word());
    bytesRead += word->ReadFromFile(fileTP);
    sp->AddWord(word);
  }
  SetSourcePhrase(sp);

  return bytesRead;
}

UINT64 TargetPhrase::ReadAlignFromFile(std::fstream &fileTPColl)
{
  UINT64 bytesRead = 0;

  UINT64 numAlign;
  fileTPColl.read((char*) &numAlign, sizeof(UINT64));
  bytesRead += sizeof(UINT64);

  for (size_t ind = 0; ind < numAlign; ++ind) {
    AlignPair alignPair;
    fileTPColl.read((char*) &alignPair.first, sizeof(UINT64));
    fileTPColl.read((char*) &alignPair.second, sizeof(UINT64));
    m_align.push_back(alignPair);

    bytesRead += sizeof(UINT64) * 2;
  }

  return bytesRead;
}

UINT64 TargetPhrase::ReadScoresFromFile(std::fstream &fileTPColl)
{
  UTIL_THROW_IF2(m_scores.size() == 0, "Translation rules must must have some scores");

  UINT64 bytesRead = 0;

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

