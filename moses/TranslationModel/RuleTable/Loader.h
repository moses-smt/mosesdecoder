/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

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

#include "Trie.h"
#include "moses/TypeDef.h"
#include "moses/parameters/AllOptions.h"

#include <istream>
#include <vector>

namespace Moses
{

/** Abstract base class defining RuleTableLoader interface.  Friend of RuleTableTrie.
 */
class RuleTableLoader
{
public:
  virtual ~RuleTableLoader() {}

  virtual bool Load(AllOptions const& opts,
                    const std::vector<FactorType> &input,
                    const std::vector<FactorType> &output,
                    const std::string &inFile,
                    size_t tableLimit,
                    RuleTableTrie &) = 0;

protected:
  // Provide access to RuleTableTrie's private SortAndPrune function.
  void SortAndPrune(RuleTableTrie &ruleTable) {
    ruleTable.SortAndPrune();
  }

  // Provide access to RuleTableTrie's private
  // GetOrCreateTargetPhraseCollection function.
  TargetPhraseCollection::shared_ptr
  GetOrCreateTargetPhraseCollection(RuleTableTrie &ruleTable,
                                    const Phrase &source,
                                    const TargetPhrase &target,
                                    const Word *sourceLHS) {
    return ruleTable.GetOrCreateTargetPhraseCollection(source, target,
           sourceLHS);
  }
};

}  // namespace Moses
