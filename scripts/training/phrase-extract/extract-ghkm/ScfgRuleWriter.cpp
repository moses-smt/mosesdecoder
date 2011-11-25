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

#include "ScfgRuleWriter.h"

#include "Alignment.h"
#include "Options.h"
#include "ScfgRule.h"

#include <cassert>
#include <ostream>
#include <map>
#include <sstream>
#include <vector>

namespace Moses {
namespace GHKM {

void ScfgRuleWriter::Write(const ScfgRule &rule)
{
  if (m_options.unpairedExtractFormat) {
    WriteUnpairedFormat(rule);
  } else {
    WriteStandardFormat(rule);
  }
}

void ScfgRuleWriter::WriteStandardFormat(const ScfgRule &rule)
{
  const std::vector<Symbol> &sourceRHS = rule.GetSourceRHS();
  const std::vector<Symbol> &targetRHS = rule.GetTargetRHS();

  std::map<int, int> sourceToTargetNTMap;
  std::map<int, int> targetToSourceNTMap;

  const Alignment &alignment = rule.GetAlignment();

  for (Alignment::const_iterator p(alignment.begin());
       p != alignment.end(); ++p) {
    if (sourceRHS[p->first].GetType() == NonTerminal) {
      assert(targetRHS[p->second].GetType() == NonTerminal);
      sourceToTargetNTMap[p->first] = p->second;
      targetToSourceNTMap[p->second] = p->first;
    }
  }

  std::ostringstream sourceSS;
  std::ostringstream targetSS;

  // Write the source side of the rule to sourceSS.
  int i = 0;
  for (std::vector<Symbol>::const_iterator p(sourceRHS.begin());
       p != sourceRHS.end(); ++p, ++i) {
    WriteSymbol(*p, sourceSS);
    if (p->GetType() == NonTerminal) {
      int targetIndex = sourceToTargetNTMap[i];
      WriteSymbol(targetRHS[targetIndex], sourceSS);
    }
    sourceSS << " ";
  }
  WriteSymbol(rule.GetSourceLHS(), sourceSS);

  // Write the target side of the rule to targetSS.
  i = 0;
  for (std::vector<Symbol>::const_iterator p(targetRHS.begin());
       p != targetRHS.end(); ++p, ++i) {
    if (p->GetType() == NonTerminal) {
      int sourceIndex = targetToSourceNTMap[i];
      WriteSymbol(sourceRHS[sourceIndex], targetSS);
    }
    WriteSymbol(*p, targetSS);
    targetSS << " ";
  }
  WriteSymbol(rule.GetTargetLHS(), targetSS);

  // Write the rule to the forward and inverse extract files.
  m_fwd << sourceSS.str() << " ||| " << targetSS.str() << " |||";
  m_inv << targetSS.str() << " ||| " << sourceSS.str() << " |||";
  for (Alignment::const_iterator p(alignment.begin());
       p != alignment.end(); ++p) {
    m_fwd << " " << p->first << "-" << p->second;
    m_inv << " " << p->second << "-" << p->first;
  }
  m_fwd << " ||| 1" << std::endl;
  m_inv << " ||| 1" << std::endl;
}

void ScfgRuleWriter::WriteUnpairedFormat(const ScfgRule &rule)
{
  const std::vector<Symbol> &sourceRHS = rule.GetSourceRHS();
  const std::vector<Symbol> &targetRHS = rule.GetTargetRHS();
  const Alignment &alignment = rule.GetAlignment();

  std::ostringstream sourceSS;
  std::ostringstream targetSS;

  // Write the source side of the rule to sourceSS.
  int i = 0;
  for (std::vector<Symbol>::const_iterator p(sourceRHS.begin());
       p != sourceRHS.end(); ++p, ++i) {
    WriteSymbol(*p, sourceSS);
    sourceSS << " ";
  }
  WriteSymbol(rule.GetSourceLHS(), sourceSS);

  // Write the target side of the rule to targetSS.
  i = 0;
  for (std::vector<Symbol>::const_iterator p(targetRHS.begin());
       p != targetRHS.end(); ++p, ++i) {
    WriteSymbol(*p, targetSS);
    targetSS << " ";
  }
  WriteSymbol(rule.GetTargetLHS(), targetSS);

  // Write the rule to the forward and inverse extract files.
  m_fwd << sourceSS.str() << " ||| " << targetSS.str() << " |||";
  m_inv << targetSS.str() << " ||| " << sourceSS.str() << " |||";
  for (Alignment::const_iterator p(alignment.begin());
       p != alignment.end(); ++p) {
    m_fwd << " " << p->first << "-" << p->second;
    m_inv << " " << p->second << "-" << p->first;
  }
  m_fwd << " ||| 1" << std::endl;
  m_inv << " ||| 1" << std::endl;
}

void ScfgRuleWriter::WriteSymbol(const Symbol &symbol, std::ostream &out)
{
  if (symbol.GetType() == NonTerminal) {
    out << "[" << symbol.GetValue() << "]";
  } else {
    out << symbol.GetValue();
  }
}

}  // namespace GHKM
}  // namespace Moses
