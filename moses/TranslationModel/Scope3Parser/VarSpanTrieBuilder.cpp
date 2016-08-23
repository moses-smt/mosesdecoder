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

#include "VarSpanTrieBuilder.h"

#include "ApplicableRuleTrie.h"
#include "IntermediateVarSpanNode.h"
#include "VarSpanNode.h"

#include <algorithm>
#include <vector>

namespace Moses
{

std::auto_ptr<VarSpanNode> VarSpanTrieBuilder::Build(
  ApplicableRuleTrie &root)
{
  std::auto_ptr<VarSpanNode> vstRoot(new VarSpanNode());
  NodeVec vec;
  const std::vector<ApplicableRuleTrie*> &children = root.m_children;
  for (std::vector<ApplicableRuleTrie*>::const_iterator p = children.begin();
       p != children.end(); ++p) {
    Build(**p, vec, *(vstRoot.get()));
  }
  return vstRoot;
}

void VarSpanTrieBuilder::Build(ApplicableRuleTrie &artNode,
                               NodeVec &vec,
                               VarSpanNode &vstRoot)
{
  typedef IntermediateVarSpanNode::Range Range;

  // Record enough information about vec that any changes made during this
  // function call can be undone at the end.
  NodeVecState state;
  RecordState(vec, state);

  if (artNode.m_end == -1) {
    if (!vec.empty() && vec.back().isOpen()) {
      ++(vec.back().m_numSplitPoints);
      ++(vec.back().m_end.first);
    } else if (artNode.m_start == -1) {
      Range start(0, -1);
      Range end(0, -1);
      vec.push_back(IntermediateVarSpanNode(start, end));
    } else {
      Range start(artNode.m_start, artNode.m_start);
      Range end(artNode.m_start, -1);
      vec.push_back(IntermediateVarSpanNode(start, end));
    }
  } else if (!vec.empty() && vec.back().isOpen()) {
    vec.back().m_end = Range(artNode.m_start-1, artNode.m_start-1);
    if (vec.back().m_start.second == -1) {
      size_t s = artNode.m_start - (vec.back().m_numSplitPoints + 1);
      vec.back().m_start.second = s;
    }
  }

  if (artNode.m_node->HasRules()) {
    artNode.m_vstNode = &(vstRoot.Insert(vec));
  }

  const std::vector<ApplicableRuleTrie*> &children = artNode.m_children;
  for (std::vector<ApplicableRuleTrie*>::const_iterator p = children.begin();
       p != children.end(); ++p) {
    Build(**p, vec, vstRoot);
  }

  // Return vec to its original value.
  RestoreState(state, vec);
}

void VarSpanTrieBuilder::RecordState(const NodeVec &vec, NodeVecState &state)
{
  state.m_size = vec.size();
  if (!vec.empty()) {
    state.m_lastNode = vec.back();
  }
}

void VarSpanTrieBuilder::RestoreState(const NodeVecState &state, NodeVec &vec)
{
  assert(state.m_size == vec.size() || state.m_size+1 == vec.size());
  if (state.m_size < vec.size()) {
    vec.resize(state.m_size);
  } else if (!vec.empty()) {
    vec.back() = state.m_lastNode;
  }
}

}
