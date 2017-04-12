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
#include <string>
#include <fstream>
#include "OnDiskWrapper.h"
#include "Vocab.h"
#include "moses/Util.h"
#include "util/exception.hh"

using namespace std;

namespace OnDiskPt
{

bool Vocab::Load(OnDiskWrapper &onDiskWrapper)
{
  fstream &file = onDiskWrapper.GetFileVocab();

  string line;
  while(getline(file, line)) {
    vector<string> tokens;
    Moses::Tokenize(tokens, line);
    UTIL_THROW_IF2(tokens.size() != 2, "Vocab file corrupted");
    const string &key = tokens[0];
    m_vocabColl[key] =  Moses::Scan<uint64_t>(tokens[1]);
  }

  // create lookup
  // assume contiguous vocab id
  m_lookup.resize(m_vocabColl.size() + 1);
  m_nextId = m_lookup.size();

  CollType::const_iterator iter;
  for (iter = m_vocabColl.begin(); iter != m_vocabColl.end(); ++iter) {
    uint32_t vocabId = iter->second;
    const std::string &word = iter->first;

    m_lookup[vocabId] = word;
  }

  return true;
}

void Vocab::Save(OnDiskWrapper &onDiskWrapper)
{
  fstream &file = onDiskWrapper.GetFileVocab();
  CollType::const_iterator iterVocab;
  for (iterVocab = m_vocabColl.begin(); iterVocab != m_vocabColl.end(); ++iterVocab) {
    const string &word = iterVocab->first;
    uint32_t vocabId = iterVocab->second;

    file << word << " " << vocabId << endl;
  }
}

uint64_t Vocab::AddVocabId(const std::string &str)
{
  // find string id
  CollType::const_iterator iter = m_vocabColl.find(str);
  if (iter == m_vocabColl.end()) {
    // add new vocab entry
    m_vocabColl[str] = m_nextId;
    return m_nextId++;
  } else {
    // return existing entry
    return iter->second;
  }
}

uint64_t Vocab::GetVocabId(const std::string &str, bool &found) const
{
  // find string id
  CollType::const_iterator iter = m_vocabColl.find(str);
  if (iter == m_vocabColl.end()) {
    found = false;
    return 0; //return whatever
  } else {
    // return existing entry
    found = true;
    return iter->second;
  }
}

}
