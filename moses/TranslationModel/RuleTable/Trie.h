/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2012 University of Edinburgh

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

#pragma once

#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/TypeDef.h"

#include <string>
#include <vector>

namespace Moses
{

class Phrase;
class TargetPhrase;
class TargetPhraseCollection;
class Word;

/*** Implementation of a SCFG rule table in a trie.  Looking up a rule of
 * length n symbols requires n look-ups to find the TargetPhraseCollection.
 * @todo why need this and PhraseDictionaryMemory?
 */
class RuleTableTrie : public PhraseDictionary
{
public:
  RuleTableTrie(const std::string &line)
    : PhraseDictionary(line, true) {
  }

  virtual ~RuleTableTrie();

  void Load(AllOptions::ptr const& opts);

private:
  friend class RuleTableLoader;

  virtual TargetPhraseCollection::shared_ptr
  GetOrCreateTargetPhraseCollection(const Phrase &source,
                                    const TargetPhrase &target,
                                    const Word *sourceLHS) = 0;

  virtual void SortAndPrune() = 0;

};

}  // namespace Moses
