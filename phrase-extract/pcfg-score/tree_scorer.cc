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

#include "tree_scorer.h"

#include <cassert>

namespace Moses {
namespace PCFG {

TreeScorer::TreeScorer(const Pcfg &pcfg, const Vocabulary &non_term_vocab)
    : pcfg_(pcfg)
    , non_term_vocab_(non_term_vocab) {
}

bool TreeScorer::Score(PcfgTree &root) const {
  if (root.IsPreterminal() || root.IsLeaf()) {
    return true;
  }

  const std::vector<PcfgTree *> &children = root.children();

  double log_prob = 0.0;

  std::vector<std::size_t> key;
  key.reserve(children.size()+1);
  key.push_back(non_term_vocab_.Lookup(root.label()));

  for (std::vector<PcfgTree *>::const_iterator p(children.begin());
       p != children.end(); ++p) {
    PcfgTree *child = *p;
    assert(!child->IsLeaf());
    key.push_back(non_term_vocab_.Lookup(child->label()));
    if (!Score(*child)) {
      return false;
    }
    if (!child->IsPreterminal()) {
      log_prob += child->score();
    }
  }
  double rule_score;
  bool found = pcfg_.Lookup(key, rule_score);
  if (!found) {
    return false;
  }
  log_prob += rule_score;
  root.set_score(log_prob);
  return true;
}

}  // namespace PCFG
}  // namespace Moses
