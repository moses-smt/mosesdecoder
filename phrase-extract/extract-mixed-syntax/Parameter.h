/*
 * Parameter.h
 *
 *  Created on: 17 Feb 2014
 *      Author: hieu
 */
#pragma once

#include <string>
#include <limits>
#include <vector>

#define UNDEFINED	std::numeric_limits<int>::max()

class Parameter
{
public:
  Parameter();
  virtual ~Parameter();

  int maxSpan;
  int minSpan;
  int maxNonTerm;
  int maxHieroNonTerm;
  int maxSymbolsTarget;
  int maxSymbolsSource;
  int minHoleSource;
  int minHoleSourceSyntax;

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
  int maxScope, minScope;

  // properties
  bool spanLength;
  bool ruleLength;
  bool nonTermContext;
  bool nonTermContextTarget;
  int nonTermContextFactor;

  int numSourceFactors, numTargetFactors;

  int nonTermConsecSourceMixedSyntax;

  std::string scopeSpanStr;
  std::vector<std::pair<int,int> > scopeSpan;

  void SetScopeSpan(const std::string &str);

};

