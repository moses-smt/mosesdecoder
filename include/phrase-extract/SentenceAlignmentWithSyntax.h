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
#ifndef SENTENCEALIGNMENTWITHSYNTAX_H_INCLUDED_
#define SENTENCEALIGNMENTWITHSYNTAX_H_INCLUDED_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "RuleExtractionOptions.h"
#include "SentenceAlignment.h"
#include "SyntaxTree.h"

namespace MosesTraining
{

class SentenceAlignmentWithSyntax : public SentenceAlignment
{
public:
  SyntaxTree targetTree;
  SyntaxTree sourceTree;
  std::set<std::string> & m_targetLabelCollection;
  std::set<std::string> & m_sourceLabelCollection;
  std::map<std::string, int> & m_targetTopLabelCollection;
  std::map<std::string, int> & m_sourceTopLabelCollection;
  const RuleExtractionOptions & m_options;

  SentenceAlignmentWithSyntax(std::set<std::string> & tgtLabelColl,
                              std::set<std::string> & srcLabelColl,
                              std::map<std::string,int> & tgtTopLabelColl,
                              std::map<std::string,int> & srcTopLabelColl,
                              const RuleExtractionOptions & options)
    : m_targetLabelCollection(tgtLabelColl)
    , m_sourceLabelCollection(srcLabelColl)
    , m_targetTopLabelCollection(tgtTopLabelColl)
    , m_sourceTopLabelCollection(srcTopLabelColl)
    , m_options(options) {
  }

  virtual ~SentenceAlignmentWithSyntax() {}

  bool
  processTargetSentence(const char *, int, bool boundaryRules);

  bool
  processSourceSentence(const char *, int, bool boundaryRules);
};

}

#endif
