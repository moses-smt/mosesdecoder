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
#include "../moses/src/FactorCollection.h"

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
    CHECK(tokens.size() == 2);
    const string &key = tokens[0];
    m_vocabColl[key] =  Moses::Scan<UINT64>(tokens[1]);
  }

  // create lookup
  // assume contiguous vocab id
  m_lookup.resize(m_vocabColl.size() + 1);

  CollType::const_iterator iter;
  for (iter = m_vocabColl.begin(); iter != m_vocabColl.end(); ++iter) {
    UINT32 vocabId = iter->second;
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
    UINT32 vocabId = iterVocab->second;

    file << word << " " << vocabId << endl;
  }
}

UINT64 Vocab::AddVocabId(const std::string &factorString)
{
  // find string id
  CollType::const_iterator iter = m_vocabColl.find(factorString);
  if (iter == m_vocabColl.end()) {
    // add new vocab entry
    m_vocabColl[factorString] = m_nextId;
    return m_nextId++;
  } else {
    // return existing entry
    return iter->second;
  }
}

UINT64 Vocab::GetVocabId(const std::string &factorString, bool &found) const
{
  // find string id
  CollType::const_iterator iter = m_vocabColl.find(factorString);
  if (iter == m_vocabColl.end()) {
    found = false;
    return 0; //return whatever
  } else {
    // return existing entry
    found = true;
    return iter->second;
  }
}

const Moses::Factor *Vocab::GetFactor(UINT32 vocabId, Moses::FactorType factorType, Moses::FactorDirection direction, bool isNonTerminal) const
{
  string str = GetString(vocabId);
  if (isNonTerminal) {
    str = str.substr(1, str.size() - 2);
  }
  const Moses::Factor *factor = Moses::FactorCollection::Instance().AddFactor(direction, factorType, str);
  return factor;
}

}
