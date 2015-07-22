// mbot rule extraction options
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
#ifndef MBOTEXTRACTIONOPTIONS_H_INCLUDED_
#define MBOTEXTRACTIONSOPTIONS_H_INCLUDED_

struct MBOTExtractionOptions {
public:
  int maxSpan;
  int minHoleSource;
  int minHoleTarget;
  int minWords;
  int maxSymbolsTarget;
  int maxSymbolsSource;
  int maxNonTerm;
  bool gzOutput;
  bool sourceSyntax;
  bool glueGrammarFlag;
  bool mbotGrammarFlag;
  bool unknownWordLabelFlag;
  bool nonTermConsecSource;
  bool requireAlignedWord;
  bool nonTermFirstWord;

  MBOTExtractionOptions()
    : maxSpan(10)
    , minHoleSource(2)
    , minHoleTarget(2)
    , minWords(1)
    , maxSymbolsTarget(999)
    , maxSymbolsSource(5)
    , maxNonTerm(5)
  	, gzOutput(false)
  	, sourceSyntax(false)
    , glueGrammarFlag(false)
  	, mbotGrammarFlag(false)
  	, unknownWordLabelFlag(false)
  	, nonTermConsecSource(false)
  	, requireAlignedWord(true)
  	, nonTermFirstWord(true)
  {}
};

#endif
