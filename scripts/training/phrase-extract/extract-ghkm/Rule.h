/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2010 University of Edinburgh

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
#ifndef RULE_H_INCLUDED_
#define RULE_H_INCLUDED_

#include "Alignment.h"

#include <string>
#include <vector>

enum SymbolType { Terminal, NonTerminal };

class Symbol
{
public:
  Symbol(const std::string & value, SymbolType type)
    : m_value(value)
    , m_type(type)
  {}

  const std::string &
  getValue() const {
    return m_value;
  }

  SymbolType
  getType() const {
    return m_type;
  }

private:
  std::string m_value;
  SymbolType m_type;
};

class Rule
{
public:
  Rule(const Symbol & sourceLHS,
       const Symbol & targetLHS,
       const std::vector<Symbol> & sourceRHS,
       const std::vector<Symbol> & targetRHS,
       const Alignment & alignment)
    : m_sourceLHS(sourceLHS)
    , m_targetLHS(targetLHS)
    , m_sourceRHS(sourceRHS)
    , m_targetRHS(targetRHS)
    , m_alignment(alignment)
  {}

  const Symbol &
  getSourceLHS() const {
    return m_sourceLHS;
  }

  const Symbol &
  getTargetLHS() const {
    return m_targetLHS;
  }

  const std::vector<Symbol> &
  getSourceRHS() const {
    return m_sourceRHS;
  }

  const std::vector<Symbol> &
  getTargetRHS() const {
    return m_targetRHS;
  }

  const Alignment &
  getAlignment() const {
    return m_alignment;
  }

private:
  Symbol m_sourceLHS;
  Symbol m_targetLHS;
  std::vector<Symbol> m_sourceRHS;
  std::vector<Symbol> m_targetRHS;
  Alignment m_alignment;
};

#endif
