/*
 * SentenceWithCandidates.h
 *
 *  Created on: 14 Dec 2015
 *      Author: hieu
 */
#pragma once

#include <boost/property_tree/ptree.hpp>
#include <string>
#include "PhraseImpl.h"
#include "Sentence.h"
#include "../MemPool.h"
#include "../pugixml.hpp"
#include "../legacy/Util2.h"

namespace Moses2
{
class FactorCollection;
class System;

class SentenceWithCandidates: public Sentence
{
public:

  static const std::string INPUT_PART_DELIM;
  static const std::string PT_LINE_DELIM;

  static SentenceWithCandidates *CreateFromString(MemPool &pool, FactorCollection &vocab,
                                    const System &system, const std::string &str);

  SentenceWithCandidates(MemPool &pool, size_t size);
  virtual ~SentenceWithCandidates();

  virtual std::string Debug(const System &system) const;
  std::string virtual getPhraseTableString() const{
    return m_phraseTableString; 
  }

private:
  std::string m_phraseTableString;

};

} /* namespace Moses2 */

