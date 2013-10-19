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
#ifndef EXTRACT_GHKM_OPTIONS_H_
#define EXTRACT_GHKM_OPTIONS_H_

#include <string>

namespace Moses
{
namespace GHKM
{

struct Options {
public:
  Options()
    : allowUnary(false)
    , conditionOnTargetLhs(false)
    , gzOutput(false)
    , maxNodes(15)
    , maxRuleDepth(3)
    , maxRuleSize(3)
    , maxScope(3)
    , minimal(false)
    , pcfg(false)
    , treeFragments(false)
    , sentenceOffset(0)
    , unpairedExtractFormat(false)
    , unknownWordMinRelFreq(0.03f)
    , unknownWordUniform(false) {}

  // Positional options
  std::string targetFile;
  std::string sourceFile;
  std::string alignmentFile;
  std::string extractFile;

  // All other options
  bool allowUnary;
  bool conditionOnTargetLhs;
  std::string glueGrammarFile;
  bool gzOutput;
  int maxNodes;
  int maxRuleDepth;
  int maxRuleSize;
  int maxScope;
  bool minimal;
  bool pcfg;
  bool treeFragments;
  int sentenceOffset;
  bool unpairedExtractFormat;
  std::string unknownWordFile;
  float unknownWordMinRelFreq;
  bool unknownWordUniform;
};

}  // namespace GHKM
}  // namespace Moses

#endif
