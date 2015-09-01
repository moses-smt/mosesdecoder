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

#pragma once
#ifndef PCFG_EXTRACT_RULE_EXTRACTOR_H_
#define PCFG_EXTRACT_RULE_EXTRACTOR_H_

#include "SyntaxTree.h"

#include "syntax-common/vocabulary.h"

#include "rule_collection.h"

namespace MosesTraining
{
namespace Syntax
{
namespace PCFG
{

// Extracts PCFG rules from syntax trees and adds them to a RuleCollection.
class RuleExtractor
{
public:
  RuleExtractor(Vocabulary &);
  void Extract(const SyntaxTree &, RuleCollection &) const;
private:
  Vocabulary &non_term_vocab_;
};

}  // namespace PCFG
}  // namespace Syntax
}  // namespace MosesTraining

#endif
