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

namespace MosesTraining
{
namespace Syntax
{
namespace GHKM
{

struct Options {
public:
  Options()
    : allowUnary(false)
    , conditionOnTargetLhs(false)
    , gzOutput(false)
    , includeSentenceId(false)
    , maxNodes(15)
    , maxRuleDepth(3)
    , maxRuleSize(3)
    , maxScope(3)
    , minimal(false)
    , partsOfSpeech(false)
    , partsOfSpeechFactor(false)
    , pcfg(false)
    , phraseOrientation(false)
    , sentenceOffset(0)
    , sourceLabels(false)
    , stripBitParLabels(false)
    , stsg(false)
    , t2s(false)
    , treeFragments(false)
    , unknownWordMinRelFreq(0.03f)
    , unknownWordUniform(false)
    , unpairedExtractFormat(false) {}

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
  bool includeSentenceId;
  int maxNodes;
  int maxRuleDepth;
  int maxRuleSize;
  int maxScope;
  bool minimal;
  bool partsOfSpeech;
  bool partsOfSpeechFactor;
  bool pcfg;
  bool phraseOrientation;
  int sentenceOffset;
  bool sourceLabels;
  std::string sourceLabelSetFile;
  std::string sourceUnknownWordFile;
  bool stripBitParLabels;
  bool stsg;
  bool t2s;
  std::string targetUnknownWordFile;
  bool treeFragments;
  float unknownWordMinRelFreq;
  std::string unknownWordSoftMatchesFile;
  bool unknownWordUniform;
  bool unpairedExtractFormat;
};

}  // namespace GHKM
}  // namespace Syntax
}  // namespace MosesTraining
