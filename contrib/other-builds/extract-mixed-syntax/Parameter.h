/*
 * Parameter.h
 *
 *  Created on: 17 Feb 2014
 *      Author: hieu
 */
#pragma once

#include <string>
#include <limits>

#define UNDEFINED	std::numeric_limits<int>::max()

class Parameter
{
public:
  Parameter();
  virtual ~Parameter();

  int maxSpan;
  int maxNonTerm;
  int maxHieroNonTerm;
  int maxSymbolsTarget;
  int maxSymbolsSource;
  int minHoleSource;

  long sentenceOffset;

  bool nonTermConsecSource;
  bool requireAlignedWord;
  bool fractionalCounting;
  bool gzOutput;

  std::string hieroNonTerm;
  std::string gluePath;

  bool sourceSyntax, targetSyntax;

  int mixedSyntaxType, multiLabel;
  bool nonTermConsecSourceMixed;
  bool hieroSourceLHS;
  int maxSpanFreeNonTermSource;
  bool nieceTerminal;
  bool spanLength;
  int maxScope;

};

