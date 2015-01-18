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
#ifndef EXTRACT_GHKM_RULE_WRITER_H_
#define EXTRACT_GHKM_RULE_WRITER_H_

#include "Subgraph.h"

#include <ostream>

namespace Moses
{
namespace GHKM
{

struct Options;
class ScfgRule;
class Symbol;

class ScfgRuleWriter
{
public:
  ScfgRuleWriter(std::ostream &fwd, std::ostream &inv, const Options &options)
    : m_fwd(fwd)
    , m_inv(inv)
    , m_options(options) {}

  void Write(const ScfgRule &rule, bool printEndl=true);

  void Write(const ScfgRule &rule, const Subgraph &g, bool printEndl=true);

private:
  // Disallow copying
  ScfgRuleWriter(const ScfgRuleWriter &);
  ScfgRuleWriter &operator=(const ScfgRuleWriter &);

  void WriteStandardFormat(const ScfgRule &, std::ostream &, std::ostream &);
  void WriteUnpairedFormat(const ScfgRule &, std::ostream &, std::ostream &);
  void WriteSymbol(const Symbol &, std::ostream &);

  std::ostream &m_fwd;
  std::ostream &m_inv;
  const Options &m_options;
};

}  // namespace GHKM
}  // namespace Moses

#endif
