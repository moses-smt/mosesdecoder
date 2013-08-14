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
#ifndef EXTRACT_GHKM_COMPOSED_RULE_H_
#define EXTRACT_GHKM_COMPOSED_RULE_H_

#include "Subgraph.h"

#include <vector>
#include <queue>

namespace Moses
{
namespace GHKM
{

class Node;
struct Options;

class ComposedRule
{
public:
  // Form a 'trivial' ComposedRule from a single existing rule.
  ComposedRule(const Subgraph &baseRule);

  // Returns the first open attachment point if any exist or 0 otherwise.
  const Node *GetOpenAttachmentPoint();

  // Close the first open attachment point without attaching a rule.
  void CloseAttachmentPoint();

  // Attempts to produce a new composed rule by attaching a given rule at the
  // first open attachment point.  This will fail if the proposed rule violates
  // the constraints set in the Options object, in which case the function
  // returns 0.
  ComposedRule *AttemptComposition(const Subgraph &, const Options &) const;

  // Constructs a Subgraph object corresponding to the composed rule.
  Subgraph CreateSubgraph();

private:
  ComposedRule(const ComposedRule &, const Subgraph &, int);

  const Subgraph &m_baseRule;
  std::vector<const Subgraph *> m_attachedRules;
  std::queue<const Node *> m_openAttachmentPoints;
  int m_depth;
  int m_size;
  int m_nodeCount;
};

}  // namespace GHKM
}  // namespace Moses

#endif
