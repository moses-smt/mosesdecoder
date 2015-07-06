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
#ifndef EXTRACT_GHKM_ALIGNMENT_GRAPH_H_
#define EXTRACT_GHKM_ALIGNMENT_GRAPH_H_

#include <set>
#include <string>
#include <vector>

#include "SyntaxTree.h"

#include "Alignment.h"
#include "Options.h"

namespace MosesTraining
{
namespace Syntax
{
namespace GHKM
{

class Node;
class Subgraph;

class AlignmentGraph
{
public:
  AlignmentGraph(const SyntaxTree *,
                 const std::vector<std::string> &,
                 const Alignment &);

  ~AlignmentGraph();

  Node *GetRoot() {
    return m_root;
  }
  const std::vector<Node *> &GetTargetNodes() {
    return m_targetNodes;
  }

  void ExtractMinimalRules(const Options &);
  void ExtractComposedRules(const Options &);

private:
  // Disallow copying
  AlignmentGraph(const AlignmentGraph &);
  AlignmentGraph &operator=(const AlignmentGraph &);

  Node *CopyParseTree(const SyntaxTree *);
  void ComputeFrontierSet(Node *, const Options &, std::set<Node *> &) const;
  bool IsFrontierNode(const Node &, const Options &) const;
  void CalcComplementSpans(Node *);
  void GetTargetTreeLeaves(Node *, std::vector<Node *> &);
  void AttachUnalignedSourceWords();
  Node *DetermineAttachmentPoint(int);
  Subgraph ComputeMinimalFrontierGraphFragment(Node *,
      const std::set<Node *> &);
  void ExtractComposedRules(Node *, const Options &);

  Node *m_root;
  std::vector<Node *> m_sourceNodes;
  std::vector<Node *> m_targetNodes;
};

}  // namespace GHKM
}  // namespace Syntax
}  // namespace MosesTraining

#endif
