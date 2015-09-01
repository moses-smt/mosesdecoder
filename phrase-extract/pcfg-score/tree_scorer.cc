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
#include <sstream>

namespace MosesTraining
{
namespace Syntax
{
namespace PCFG
{

TreeScorer::TreeScorer(const Pcfg &pcfg, const Vocabulary &non_term_vocab)
  : pcfg_(pcfg)
  , non_term_vocab_(non_term_vocab)
{
}

bool TreeScorer::Score(SyntaxTree &root)
{
  scores_.clear();
  ZeroScores(root);
  if (!CalcScores(root)) {
    return false;
  }
  SetAttributes(root);
  return true;
}

bool TreeScorer::CalcScores(SyntaxTree &root)
{
  if (root.IsLeaf() || root.children()[0]->IsLeaf()) {
    return true;
  }

  const std::vector<SyntaxTree *> &children = root.children();

  double log_prob = 0.0;

  std::vector<std::size_t> key;
  key.reserve(children.size()+1);
  key.push_back(non_term_vocab_.Lookup(root.value().label));

  for (std::vector<SyntaxTree *>::const_iterator p(children.begin());
       p != children.end(); ++p) {
    SyntaxTree *child = *p;
    assert(!child->IsLeaf());
    key.push_back(non_term_vocab_.Lookup(child->value().label));
    if (!CalcScores(*child)) {
      return false;
    }
    if (!child->children()[0]->IsLeaf()) {
      log_prob += scores_[child];
    }
  }
  double rule_score;
  bool found = pcfg_.Lookup(key, rule_score);
  if (!found) {
    return false;
  }
  log_prob += rule_score;
  scores_[&root] = log_prob;
  return true;
}

void TreeScorer::SetAttributes(SyntaxTree &root)
{
  // Terminals don't need attributes.
  if (root.IsLeaf()) {
    return;
  }
  // Preterminals don't need attributes (they have the implicit score 0.0).
  if (root.children()[0]->IsLeaf()) {
    return;
  }
  double score = scores_[&root];
  if (score != 0.0) {
    std::ostringstream out;
    out << score;
    root.value().attributes["pcfg"] = out.str();
  }
  for (std::vector<SyntaxTree *>::const_iterator p(root.children().begin());
       p != root.children().end(); ++p) {
    SetAttributes(**p);
  }
}

void TreeScorer::ZeroScores(SyntaxTree &root)
{
  scores_[&root] = 0.0f;
  const std::vector<SyntaxTree *> &children = root.children();
  for (std::vector<SyntaxTree *>::const_iterator p(children.begin());
       p != children.end(); ++p) {
    ZeroScores(**p);
  }
}

}  // namespace PCFG
}  // namespace Syntax
}  // namespace MosesTraining
