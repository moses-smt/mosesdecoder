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

#include "rule_extractor.h"

#include "pcfg-common/pcfg_tree.h"

namespace Moses {
namespace PCFG {

RuleExtractor::RuleExtractor(Vocabulary &non_term_vocab)
    : non_term_vocab_(non_term_vocab) {
}

void RuleExtractor::Extract(const PcfgTree &tree, RuleCollection &rc) const {
  if (tree.IsPreterminal() || tree.IsLeaf()) {
    return;
  }

  std::size_t lhs = non_term_vocab_.Insert(tree.label());
  std::vector<std::size_t> rhs;

  const std::vector<PcfgTree *> &children = tree.children();
  rhs.reserve(children.size());
  for (std::vector<PcfgTree *>::const_iterator p(children.begin());
       p != children.end(); ++p) {
    const PcfgTree &child = **p;
    rhs.push_back(non_term_vocab_.Insert(child.label()));
    Extract(child, rc);
  }
  rc.Add(lhs, rhs);
}

}  // namespace PCFG
}  // namespace Moses
