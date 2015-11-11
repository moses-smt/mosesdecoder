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

#include <cassert>
#include <cmath>
#include <ostream>
#include <map>
#include <sstream>
#include <vector>

#include "Alignment.h"
#include "Options.h"
#include "ScfgRule.h"

namespace MosesTraining
{
namespace Syntax
{
namespace GHKM
{

void ScfgRuleWriter::Write(const ScfgRule &rule, size_t lineNum, bool printEndl)
{
  std::ostringstream sourceSS;
  std::ostringstream targetSS;

  if (m_options.unpairedExtractFormat) {
    WriteUnpairedFormat(rule, sourceSS, targetSS);
  } else {
    WriteStandardFormat(rule, sourceSS, targetSS);
  }

  // Write the rule to the forward and inverse extract files.
  if (m_options.t2s) {
    // If model is tree-to-string then flip the source and target.
    m_fwd << targetSS.str() << " ||| " << sourceSS.str() << " |||";
    m_inv << sourceSS.str() << " ||| " << targetSS.str() << " |||";
  } else {
    m_fwd << sourceSS.str() << " ||| " << targetSS.str() << " |||";
    m_inv << targetSS.str() << " ||| " << sourceSS.str() << " |||";
  }

  const Alignment &alignment = rule.GetAlignment();
  for (Alignment::const_iterator p = alignment.begin();
       p != alignment.end(); ++p) {
    if (m_options.t2s) {
      // If model is tree-to-string then flip the source and target.
      m_fwd << " " << p->second << "-" << p->first;
      m_inv << " " << p->first << "-" << p->second;
    } else {
      m_fwd << " " << p->first << "-" << p->second;
      m_inv << " " << p->second << "-" << p->first;
    }
  }

  if (m_options.includeSentenceId) {
    if (m_options.t2s) {
      m_inv << " ||| " << lineNum;
    } else {
      m_fwd << " ||| " << lineNum;
    }
  }

  // Write a count of 1.
  m_fwd << " ||| 1";
  m_inv << " ||| 1";

  // Write the PCFG score (if requested).
  if (m_options.pcfg) {
    m_fwd << " ||| " << std::exp(rule.GetPcfgScore());
  }

  m_fwd << " |||";

  if (m_options.sourceLabels && rule.HasSourceLabels()) {
    m_fwd << " {{SourceLabels";
    rule.PrintSourceLabels(m_fwd);
    m_fwd << "}}";
  }

  if (printEndl) {
    m_fwd << std::endl;
    m_inv << std::endl;
  }
}

void ScfgRuleWriter::WriteStandardFormat(const ScfgRule &rule,
    std::ostream &sourceSS,
    std::ostream &targetSS)
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

  // If parts-of-speech as a factor requested: retrieve preterminals from graph fragment
  std::vector<std::string> partsOfSpeech;
  if (m_options.partsOfSpeechFactor) {
    const Subgraph &graphFragment = rule.GetGraphFragment();
    graphFragment.GetPartsOfSpeech(partsOfSpeech);
  }

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
  if (m_options.conditionOnTargetLhs) {
    WriteSymbol(rule.GetTargetLHS(), sourceSS);
  } else {
    WriteSymbol(rule.GetSourceLHS(), sourceSS);
  }

  // Write the target side of the rule to targetSS.
  i = 0;
  int targetTerminalIndex = 0;
  for (std::vector<Symbol>::const_iterator p(targetRHS.begin());
       p != targetRHS.end(); ++p, ++i) {
    if (p->GetType() == NonTerminal) {
      int sourceIndex = targetToSourceNTMap[i];
      WriteSymbol(sourceRHS[sourceIndex], targetSS);
    }
    WriteSymbol(*p, targetSS);
    // If parts-of-speech as a factor requested: write part-of-speech
    if (m_options.partsOfSpeechFactor && (p->GetType() != NonTerminal)) {
      assert(targetTerminalIndex<partsOfSpeech.size());
      targetSS << "|" << partsOfSpeech[targetTerminalIndex];
      ++targetTerminalIndex;
    }
    targetSS << " ";
  }
  WriteSymbol(rule.GetTargetLHS(), targetSS);
}

void ScfgRuleWriter::WriteUnpairedFormat(const ScfgRule &rule,
    std::ostream &sourceSS,
    std::ostream &targetSS)
{
  const std::vector<Symbol> &sourceRHS = rule.GetSourceRHS();
  const std::vector<Symbol> &targetRHS = rule.GetTargetRHS();

  // If parts-of-speech as a factor requested: retrieve preterminals from graph fragment
  std::vector<std::string> partsOfSpeech;
  if (m_options.partsOfSpeechFactor) {
    const Subgraph &graphFragment = rule.GetGraphFragment();
    graphFragment.GetPartsOfSpeech(partsOfSpeech);
  }

  // Write the source side of the rule to sourceSS.
  for (std::vector<Symbol>::const_iterator p(sourceRHS.begin());
       p != sourceRHS.end(); ++p) {
    WriteSymbol(*p, sourceSS);
    sourceSS << " ";
  }
  if (m_options.conditionOnTargetLhs) {
    WriteSymbol(rule.GetTargetLHS(), sourceSS);
  } else {
    WriteSymbol(rule.GetSourceLHS(), sourceSS);
  }

  // Write the target side of the rule to targetSS.
  int targetTerminalIndex = 0;
  for (std::vector<Symbol>::const_iterator p(targetRHS.begin());
       p != targetRHS.end(); ++p) {
    WriteSymbol(*p, targetSS);
    // If parts-of-speech as a factor requested: write part-of-speech
    if (m_options.partsOfSpeechFactor && (p->GetType() != NonTerminal)) {
      assert(targetTerminalIndex<partsOfSpeech.size());
      targetSS << "|" << partsOfSpeech[targetTerminalIndex];
      ++targetTerminalIndex;
    }
    targetSS << " ";
  }
  WriteSymbol(rule.GetTargetLHS(), targetSS);
}

void ScfgRuleWriter::WriteSymbol(const Symbol &symbol, std::ostream &out)
{
  if (symbol.GetType() == NonTerminal) {
    out << "[";
    if (m_options.stripBitParLabels) {
      size_t pos = symbol.GetValue().find('-');
      if (pos == std::string::npos) {
        out << symbol.GetValue();
      } else {
        out << symbol.GetValue().substr(0,pos);
      }
    } else {
      out << symbol.GetValue();
    }
    out << "]";
  } else {
    out << symbol.GetValue();
  }
}

}  // namespace GHKM
}  // namespace Syntax
}  // namespace MosesTraining
