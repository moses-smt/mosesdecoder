/*
 * StoreVocab.h
 *
 *  Created on: 15 Jun 2016
 *      Author: hieu
 */
#pragma once
#include <string>
#include <boost/unordered_map.hpp>
#include "OutputFileStream.h"
#include "moses2/legacy/Util2.h"

namespace probingpt
{

template<typename VOCABID>
class StoreVocab
{
protected:
  std::string m_path;

  typedef boost::unordered_map<std::string, VOCABID> Coll;
  Coll m_vocab;

public:
  StoreVocab(const std::string &path)
    :m_path(path)
  {}

  virtual ~StoreVocab() {}

  VOCABID GetVocabId(const std::string &word) {
    typename Coll::iterator iter = m_vocab.find(word);
    if (iter == m_vocab.end()) {
      VOCABID ind = m_vocab.size() + 1;
      m_vocab[word] = ind;
      return ind;
    } else {
      return iter->second;
    }
  }

  void Insert(VOCABID id, const std::string &word) {
    m_vocab[word] = id;
  }

  void Save() {
    OutputFileStream strme(m_path);

    typename Coll::const_iterator iter;
    for (iter = m_vocab.begin(); iter != m_vocab.end(); ++iter) {
      strme << iter->first << "\t" << iter->second << std::endl;
    }

    strme.Close();
  }
};

} /* namespace Moses2 */

