/*
 * Sentence.h
 *
 *  Created on: 14 Dec 2015
 *      Author: hieu
 */
#pragma once

#include <boost/property_tree/ptree.hpp>
#include <string>
#include "PhraseImpl.h"
#include "../InputType.h"
#include "../MemPool.h"
#include "../pugixml.hpp"
#include "../legacy/Util2.h"

namespace Moses2
{
class FactorCollection;
class System;

class Sentence: public InputType, public PhraseImpl
{
public:

  //////////////////////////////////////////////////////////////////////////////
  class XMLOption
  {
    friend std::ostream& operator<<(std::ostream &out, const XMLOption &obj);

  public:
    std::string nodeName;
    size_t startPos, phraseSize;
  };

  //////////////////////////////////////////////////////////////////////////////

  static Sentence *CreateFromString(MemPool &pool, FactorCollection &vocab,
      const System &system, const std::string &str, long translationId);

  Sentence(long translationId, MemPool &pool, size_t size)
  :InputType(translationId)
  ,PhraseImpl(pool, size)
  {}

  virtual ~Sentence()
  {}

protected:
  static void XMLParse(
      size_t depth,
      const pugi::xml_node &parentNode,
      std::vector<std::string> &toks,
      std::vector<XMLOption*> &xmlOptions);

};

} /* namespace Moses2 */

