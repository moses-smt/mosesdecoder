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

#include "moses/NonTerminal.h"
#include "moses/TranslationModel/Scope3Parser/Parser.h"
#include "moses/StaticData.h"
#include "moses/TargetPhrase.h"
#include "moses/TargetPhraseCollection.h"
#include "moses/Util.h"
#include "moses/Word.h"
#include "UTrie.h"
#include "Trie.h"
#include "UTrieNode.h"

#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <boost/version.hpp>

#include <map>
#include <vector>

namespace Moses
{

TargetPhraseCollection &RuleTableUTrie::GetOrCreateTargetPhraseCollection(
  const Phrase &source, const TargetPhrase &target, const Word *sourceLHS)
{
  UTrieNode &currNode = GetOrCreateNode(source, target, sourceLHS);
  return currNode.GetOrCreateTargetPhraseCollection(target);
}

UTrieNode &RuleTableUTrie::GetOrCreateNode(const Phrase &source,
    const TargetPhrase &target,
    const Word */*sourceLHS*/)
{
  const size_t size = source.GetSize();

  const AlignmentInfo &alignmentInfo = target.GetAlignNonTerm();
  AlignmentInfo::const_iterator iterAlign = alignmentInfo.begin();

  UTrieNode *currNode = &m_root;
  for (size_t pos = 0 ; pos < size ; ++pos) {
    const Word &word = source.GetWord(pos);

    if (word.IsNonTerminal()) {
      assert(iterAlign != alignmentInfo.end());
      assert(iterAlign->first == pos);
      size_t targetNonTermInd = iterAlign->second;
      ++iterAlign;
      const Word &targetNonTerm = target.GetWord(targetNonTermInd);
      currNode = currNode->GetOrCreateNonTerminalChild(targetNonTerm);
    } else {
      currNode = currNode->GetOrCreateTerminalChild(word);
    }

    assert(currNode != NULL);
  }

  return *currNode;
}

ChartRuleLookupManager *RuleTableUTrie::CreateRuleLookupManager(
  const ChartParser &parser,
  const ChartCellCollectionBase &cellCollection)
{
  // FIXME This should be a parameter to CreateRuleLookupManager
  size_t maxChartSpan = 0;
  return new Scope3Parser(parser, cellCollection, *this, maxChartSpan);
}

void RuleTableUTrie::SortAndPrune()
{
  if (GetTableLimit()) {
    m_root.Sort(GetTableLimit());
  }
}

}  // namespace Moses
