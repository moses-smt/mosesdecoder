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

namespace MosesTraining
{

struct RuleExtractionOptions {
public:
  int maxSpan;
  int minHoleSource;
  int minHoleTarget;
  int minWords;
  int maxSymbolsTarget;
  int maxSymbolsSource;
  int maxNonTerm;
  int maxScope;
  bool onlyDirectFlag;
  bool glueGrammarFlag;
  bool unknownWordLabelFlag;
  bool onlyOutputSpanInfo;
  bool noFileLimit;
  bool properConditioning;
  bool nonTermFirstWord;
  bool nonTermConsecTarget;
  bool nonTermConsecSource;
  bool requireAlignedWord;
  bool sourceSyntax;
  bool targetSyntax;
  bool targetSyntacticPreferences;
  bool duplicateRules;
  bool fractionalCounting;
  bool pcfgScore;
  bool gzOutput;
  bool unpairedExtractFormat;
  bool conditionOnTargetLhs;
  bool boundaryRules;
  bool flexScoreFlag;
  bool phraseOrientation;

  RuleExtractionOptions()
    : maxSpan(10)
    , minHoleSource(2)
    , minHoleTarget(1)
    , minWords(1)
    , maxSymbolsTarget(999)
    , maxSymbolsSource(5)
    , maxNonTerm(2)
    , maxScope(999)
    // int minHoleSize(1)
    // int minSubPhraseSize(1) // minimum size of a remaining lexical phrase
    , onlyDirectFlag(false)
    , glueGrammarFlag(false)
    , unknownWordLabelFlag(false)
    , onlyOutputSpanInfo(false)
    , noFileLimit(false)
    //bool zipFiles(false)
    , properConditioning(false)
    , nonTermFirstWord(true)
    , nonTermConsecTarget(true)
    , nonTermConsecSource(false)
    , requireAlignedWord(true)
    , sourceSyntax(false)
    , targetSyntax(false)
    , targetSyntacticPreferences(false)
    , duplicateRules(true)
    , fractionalCounting(true)
    , pcfgScore(false)
    , gzOutput(false)
    , unpairedExtractFormat(false)
    , conditionOnTargetLhs(false)
    , boundaryRules(false)
    , flexScoreFlag(false)
    , phraseOrientation(false) {}
};

}

