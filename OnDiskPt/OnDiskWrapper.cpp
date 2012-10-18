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
#ifdef WIN32
#include <direct.h>
#endif
#include <sys/stat.h>
#include "util/check.hh"
#include <string>
#include "OnDiskWrapper.h"

using namespace std;

namespace OnDiskPt
{

int OnDiskWrapper::VERSION_NUM = 5;

OnDiskWrapper::OnDiskWrapper()
{
}

OnDiskWrapper::~OnDiskWrapper()
{
  delete m_rootSourceNode;
}

bool OnDiskWrapper::BeginLoad(const std::string &filePath)
{
  if (!OpenForLoad(filePath))
    return false;

  if (!m_vocab.Load(*this))
    return false;

  UINT64 rootFilePos = GetMisc("RootNodeOffset");
  m_rootSourceNode = new PhraseNode(rootFilePos, *this);

  return true;
}

bool OnDiskWrapper::OpenForLoad(const std::string &filePath)
{
  m_fileSource.open((filePath + "/Source.dat").c_str(), ios::in | ios::binary);
  CHECK(m_fileSource.is_open());

  m_fileTargetInd.open((filePath + "/TargetInd.dat").c_str(), ios::in | ios::binary);
  CHECK(m_fileTargetInd.is_open());

  m_fileTargetColl.open((filePath + "/TargetColl.dat").c_str(), ios::in | ios::binary);
  CHECK(m_fileTargetColl.is_open());

  m_fileVocab.open((filePath + "/Vocab.dat").c_str(), ios::in);
  CHECK(m_fileVocab.is_open());

  m_fileMisc.open((filePath + "/Misc.dat").c_str(), ios::in);
  CHECK(m_fileMisc.is_open());

  // set up root node
  LoadMisc();
  m_numSourceFactors = GetMisc("NumSourceFactors");
  m_numTargetFactors = GetMisc("NumTargetFactors");
  m_numScores = GetMisc("NumScores");

  return true;
}

bool OnDiskWrapper::LoadMisc()
{
  char line[100000];

  while(m_fileMisc.getline(line, 100000)) {
    vector<string> tokens;
    Moses::Tokenize(tokens, line);
    CHECK(tokens.size() == 2);
    const string &key = tokens[0];
    m_miscInfo[key] =  Moses::Scan<UINT64>(tokens[1]);
  }

  return true;
}

bool OnDiskWrapper::BeginSave(const std::string &filePath
                              , int numSourceFactors, int	numTargetFactors, int numScores)
{
  m_numSourceFactors = numSourceFactors;
  m_numTargetFactors = numTargetFactors;
  m_numScores = numScores;
  m_filePath = filePath;

#ifdef WIN32
  mkdir(filePath.c_str());
#else
  mkdir(filePath.c_str(), 0777);
#endif

  m_fileSource.open((filePath + "/Source.dat").c_str(), ios::out | ios::in | ios::binary | ios::ate | ios::trunc);
  CHECK(m_fileSource.is_open());

  m_fileTargetInd.open((filePath + "/TargetInd.dat").c_str(), ios::out | ios::binary | ios::ate | ios::trunc);
  CHECK(m_fileTargetInd.is_open());

  m_fileTargetColl.open((filePath + "/TargetColl.dat").c_str(), ios::out | ios::binary | ios::ate | ios::trunc);
  CHECK(m_fileTargetColl.is_open());

  m_fileVocab.open((filePath + "/Vocab.dat").c_str(), ios::out | ios::ate | ios::trunc);
  CHECK(m_fileVocab.is_open());

  m_fileMisc.open((filePath + "/Misc.dat").c_str(), ios::out | ios::ate | ios::trunc);
  CHECK(m_fileMisc.is_open());

  // offset by 1. 0 offset is reserved
  char c = 0xff;
  m_fileSource.write(&c, 1);
  CHECK(1 == m_fileSource.tellp());

  m_fileTargetInd.write(&c, 1);
  CHECK(1 == m_fileTargetInd.tellp());

  m_fileTargetColl.write(&c, 1);
  CHECK(1 == m_fileTargetColl.tellp());

  // set up root node
  CHECK(GetNumCounts() == 1);
  vector<float> counts(GetNumCounts());
  counts[0] = DEFAULT_COUNT;
  m_rootSourceNode = new PhraseNode();
  m_rootSourceNode->AddCounts(counts);

  return true;
}

void OnDiskWrapper::EndSave()
{
  bool ret = m_rootSourceNode->Saved();
  CHECK(ret);

  GetVocab().Save(*this);

  SaveMisc();

  m_fileMisc.close();
  m_fileVocab.close();
  m_fileSource.close();
  m_fileTarget.close();
  m_fileTargetInd.close();
  m_fileTargetColl.close();
}

void OnDiskWrapper::SaveMisc()
{
  m_fileMisc << "Version " << VERSION_NUM << endl;
  m_fileMisc << "NumSourceFactors " << m_numSourceFactors << endl;
  m_fileMisc << "NumTargetFactors " << m_numTargetFactors << endl;
  m_fileMisc << "NumScores " << m_numScores << endl;
  m_fileMisc << "RootNodeOffset " << m_rootSourceNode->GetFilePos() << endl;
}

size_t OnDiskWrapper::GetSourceWordSize() const
{
  return sizeof(UINT64) + sizeof(char);
}

size_t OnDiskWrapper::GetTargetWordSize() const
{
  return sizeof(UINT64) + sizeof(char);
}

UINT64 OnDiskWrapper::GetMisc(const std::string &key) const
{
  std::map<std::string, UINT64>::const_iterator iter;
  iter = m_miscInfo.find(key);
  CHECK(iter != m_miscInfo.end());

  return iter->second;
}

PhraseNode &OnDiskWrapper::GetRootSourceNode()
{
  return *m_rootSourceNode;
}

Word *OnDiskWrapper::ConvertFromMoses(Moses::FactorDirection /* direction */
                                      , const std::vector<Moses::FactorType> &factorsVec
                                      , const Moses::Word &origWord) const
{
  bool isNonTerminal = origWord.IsNonTerminal();
  Word *newWord = new Word(isNonTerminal);
  stringstream strme;

  size_t factorType = factorsVec[0];  
  const Moses::Factor *factor = origWord.GetFactor(factorType);
  CHECK(factor);  
  string str = factor->GetString();
  strme << str;

  for (size_t ind = 1 ; ind < factorsVec.size() ; ++ind) {
    size_t factorType = factorsVec[ind];
    const Moses::Factor *factor = origWord.GetFactor(factorType);
    if (factor == NULL)
    { // can have less factors than factorType.size()
      break;
    }
    CHECK(factor);
    string str = factor->GetString();
    strme << "|" << str;    
  } // for (size_t factorType

  bool found;
  UINT64 vocabId = m_vocab.GetVocabId(strme.str(), found);
  if (!found) {
    // factor not in phrase table -> phrse definately not in. exit
    delete newWord;
    return NULL;
  } else {
    newWord->SetVocabId(vocabId);
    return newWord;
  }
}


}
