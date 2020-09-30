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

  static SentenceWithCandidates *CreateFromString(MemPool &pool, FactorCollection &vocab,
                                    const System &system, const std::string &str);

  SentenceWithCandidates(MemPool &pool, size_t size)
    :Sentence(pool, size)
  {}

  virtual ~SentenceWithCandidates()
  {}

protected:
  static SentenceWithCandidates *CreateFromStringXML(MemPool &pool, FactorCollection &vocab,
                                       const System &system, const std::string &str);

  static void XMLParse(
    MemPool &pool,
    const System &system,
    size_t depth,
    const pugi::xml_node &parentNode,
    std::vector<std::string> &toks,
    std::vector<XMLOption*> &xmlOptions);

};

} /* namespace Moses2 */

