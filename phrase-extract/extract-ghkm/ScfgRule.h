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

#include <string>
#include <vector>
#include <list>
#include <memory>
#include <iostream>

#include "Alignment.h"
#include "Rule.h"
#include "SyntaxNodeCollection.h"

namespace MosesTraining
{
namespace Syntax
{
namespace GHKM
{

class Node;
class Subgraph;

class ScfgRule : public Rule
{
public:
  ScfgRule(const Subgraph &fragment,
           const SyntaxNodeCollection *sourceNodeCollection = 0);

  const Subgraph &GetGraphFragment() const {
    return m_graphFragment;
  }
  const Symbol &GetSourceLHS() const {
    return m_sourceLHS;
  }
  const Symbol &GetTargetLHS() const {
    return m_targetLHS;
  }
  const std::vector<Symbol> &GetSourceRHS() const {
    return m_sourceRHS;
  }
  const std::vector<Symbol> &GetTargetRHS() const {
    return m_targetRHS;
  }
  float GetPcfgScore() const {
    return m_pcfgScore;
  }
  bool HasSourceLabels() const {
    return m_hasSourceLabels;
  }
  void PrintSourceLabels(std::ostream &out) const {
    for (std::vector<std::string>::const_iterator it = m_sourceLabels.begin();
         it != m_sourceLabels.end(); ++it) {
      out << " " << (*it);
    }
  }
  void UpdateSourceLabelCoocCounts(std::map< std::string, std::map<std::string,float>* > &coocCounts,
                                   float count) const;

  int Scope() const {
    return Rule::Scope(m_sourceRHS);
  }

private:
  void PushSourceLabel(const SyntaxNodeCollection *sourceNodeCollection,
                       const Node *node, const std::string &nonMatchingLabel);

  const Subgraph& m_graphFragment;
  Symbol m_sourceLHS;
  Symbol m_targetLHS;
  std::vector<Symbol> m_sourceRHS;
  std::vector<Symbol> m_targetRHS;
  float m_pcfgScore;
  bool m_hasSourceLabels;
  std::vector<std::string> m_sourceLabels;
  unsigned m_numberOfNonTerminals;
};

}  // namespace GHKM
}  // namespace Syntax
}  // namespace MosesTraining
