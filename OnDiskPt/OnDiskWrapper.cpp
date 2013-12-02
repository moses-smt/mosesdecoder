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
#include <string>
#include "OnDiskWrapper.h"
#include "moses/Factor.h"
#include "util/exception.hh"

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

void OnDiskWrapper::BeginLoad(const std::string &filePath)
{
  if (!OpenForLoad(filePath)) {
    UTIL_THROW(util::FileOpenException, "Couldn't open for loading: " << filePath);
  }

  if (!m_vocab.Load(*this))
    UTIL_THROW(util::FileOpenException, "Couldn't load vocab");

  UINT64 rootFilePos = GetMisc("RootNodeOffset");
  m_rootSourceNode = new PhraseNode(rootFilePos, *this);
}

bool OnDiskWrapper::OpenForLoad(const std::string &filePath)
{
  m_fileSource.open((filePath + "/Source.dat").c_str(), ios::in | ios::binary);
  UTIL_THROW_IF(!m_fileSource.is_open(),
		  util::FileOpenException,
		  "Couldn't open file " << filePath << "/Source.dat");

  m_fileTargetInd.open((filePath + "/TargetInd.dat").c_str(), ios::in | ios::binary);
  UTIL_THROW_IF(!m_fileTargetInd.is_open(),
		  util::FileOpenException,
		  "Couldn't open file " << filePath << "/TargetInd.dat");

  m_fileTargetColl.open((filePath + "/TargetColl.dat").c_str(), ios::in | ios::binary);
  UTIL_THROW_IF(!m_fileTargetColl.is_open(),
		  util::FileOpenException,
		  "Couldn't open file " << filePath << "/TargetColl.dat");

  m_fileVocab.open((filePath + "/Vocab.dat").c_str(), ios::in);
  UTIL_THROW_IF(!m_fileVocab.is_open(),
		  util::FileOpenException,
		  "Couldn't open file " << filePath << "/Vocab.dat");

  m_fileMisc.open((filePath + "/Misc.dat").c_str(), ios::in);
  UTIL_THROW_IF(!m_fileMisc.is_open(),
		  util::FileOpenException,
		  "Couldn't open file " << filePath << "/Misc.dat");

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
    UTIL_THROW_IF2(tokens.size() != 2, "Except key value. Found " << line);


    const string &key = tokens[0];
    m_miscInfo[key] =  Moses::Scan<UINT64>(tokens[1]);
  }

  return true;
}

void OnDiskWrapper::BeginSave(const std::string &filePath
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
  UTIL_THROW_IF(!m_fileSource.is_open(),
		  util::FileOpenException,
		  "Couldn't open file " << filePath << "/Source.dat");

  m_fileTargetInd.open((filePath + "/TargetInd.dat").c_str(), ios::out | ios::binary | ios::ate | ios::trunc);
  UTIL_THROW_IF(!m_fileTargetInd.is_open(),
		  util::FileOpenException,
		  "Couldn't open file " << filePath << "/TargetInd.dat");

  m_fileTargetColl.open((filePath + "/TargetColl.dat").c_str(), ios::out | ios::binary | ios::ate | ios::trunc);
  UTIL_THROW_IF(!m_fileTargetColl.is_open(),
		  util::FileOpenException,
		  "Couldn't open file " << filePath << "/TargetColl.dat");

  m_fileVocab.open((filePath + "/Vocab.dat").c_str(), ios::out | ios::ate | ios::trunc);
  UTIL_THROW_IF(!m_fileVocab.is_open(),
		  util::FileOpenException,
		  "Couldn't open file " << filePath << "/Vocab.dat");

  m_fileMisc.open((filePath + "/Misc.dat").c_str(), ios::out | ios::ate | ios::trunc);
  UTIL_THROW_IF(!m_fileMisc.is_open(),
		  util::FileOpenException,
		  "Couldn't open file " << filePath << "/Misc.dat");

  // offset by 1. 0 offset is reserved
  char c = 0xff;
  m_fileSource.write(&c, 1);
  UTIL_THROW_IF2(1 != m_fileSource.tellp(),
	"Couldn't write to stream m_fileSource");

  m_fileTargetInd.write(&c, 1);
  UTIL_THROW_IF2(1 != m_fileTargetInd.tellp(),
	  	"Couldn't write to stream m_fileTargetInd");

  m_fileTargetColl.write(&c, 1);
  UTIL_THROW_IF2(1 != m_fileTargetColl.tellp(),
		  	"Couldn't write to stream m_fileTargetColl");

  // set up root node
  UTIL_THROW_IF2(GetNumCounts() != 1,
	  	"Not sure what this is...");

  vector<float> counts(GetNumCounts());
  counts[0] = DEFAULT_COUNT;
  m_rootSourceNode = new PhraseNode();
  m_rootSourceNode->AddCounts(counts);
}

void OnDiskWrapper::EndSave()
{
  bool ret = m_rootSourceNode->Saved();
  UTIL_THROW_IF2(!ret, "Root node not saved");

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
  UTIL_THROW_IF2(iter == m_miscInfo.end()
		  	  , "Couldn't find value for key " << key
  	  	  	  );

  return iter->second;
}

Word *OnDiskWrapper::ConvertFromMoses(const std::vector<Moses::FactorType> &factorsVec
                                      , const Moses::Word &origWord) const
{
  bool isNonTerminal = origWord.IsNonTerminal();
  Word *newWord = new Word(isNonTerminal);
  stringstream strme;

  size_t factorType = factorsVec[0];
  const Moses::Factor *factor = origWord.GetFactor(factorType);
  UTIL_THROW_IF2(factor == NULL, "Expecting factor " << factorType);
  strme << factor->GetString();

  for (size_t ind = 1 ; ind < factorsVec.size() ; ++ind) {
    size_t factorType = factorsVec[ind];
    const Moses::Factor *factor = origWord.GetFactor(factorType);
    if (factor == NULL) {
      // can have less factors than factorType.size()
      break;
    }
    UTIL_THROW_IF2(factor == NULL,
    		"Expecting factor " << factorType << " at position " << ind);
    strme << "|" << factor->GetString();
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
