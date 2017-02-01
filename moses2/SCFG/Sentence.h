/*
 * Sentence.h
 *
 *  Created on: 14 Dec 2015
 *      Author: hieu
 */
#pragma once

#include <string>
#include "PhraseImpl.h"
#include "../InputType.h"
#include "../MemPool.h"
#include "../legacy/Util2.h"
#include "../pugixml.hpp"

namespace Moses2
{
class FactorCollection;
class System;

namespace SCFG
{

class Sentence: public InputType, public PhraseImpl
{
public:
  static Sentence *CreateFromString(MemPool &pool, FactorCollection &vocab,
                                    const System &system, const std::string &str, long translationId);

  Sentence(MemPool &pool, size_t size)
    :InputType(pool)
    ,PhraseImpl(pool, size)
  {}

  virtual ~Sentence()
  {}

protected:
  static Sentence *CreateFromStringXML(MemPool &pool, FactorCollection &vocab,
                                       const System &system, const std::string &str);

  static void XMLParse(
    MemPool &pool,
    const System &system,
    size_t depth,
    const pugi::xml_node &parentNode,
    std::vector<std::string> &toks,
    std::vector<XMLOption*> &xmlOptions);

};

}
} /* namespace Moses2 */

