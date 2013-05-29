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
#ifndef EXTRACT_GHKM_SCFG_RULE_H_
#define EXTRACT_GHKM_SCFG_RULE_H_

#include "Alignment.h"

#include <string>
#include <vector>

namespace Moses
{
namespace GHKM
{

class Node;
class Subgraph;

enum SymbolType { Terminal, NonTerminal };

struct Symbol {
public:
  Symbol(const std::string &v, SymbolType t) : m_value(v) , m_type(t) {}

  const std::string &GetValue() const {
    return m_value;
  }
  SymbolType GetType() const {
    return m_type;
  }

private:
  std::string m_value;
  SymbolType m_type;
};

class ScfgRule
{
public:
  ScfgRule(const Subgraph &fragment);

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
  const Alignment &GetAlignment() const {
    return m_alignment;
  }
  float GetPcfgScore() const {
    return m_pcfgScore;
  }

  int Scope() const;

private:
  static bool PartitionOrderComp(const Node *, const Node *);

  Symbol m_sourceLHS;
  Symbol m_targetLHS;
  std::vector<Symbol> m_sourceRHS;
  std::vector<Symbol> m_targetRHS;
  Alignment m_alignment;
  float m_pcfgScore;
};

}  // namespace GHKM
}  // namespace Moses

#endif
