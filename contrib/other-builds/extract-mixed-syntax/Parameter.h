/*
 * Parameter.h
 *
 *  Created on: 17 Feb 2014
 *      Author: hieu
 */
#pragma once

#include <string>

class Parameter
{
public:
  Parameter();
  virtual ~Parameter();

  int maxSpan;
  int maxNonTerm;
  int maxSymbolsTarget;
  int maxSymbolsSource;
  int minHoleSource;

  bool nonTermConsecSource;
  bool requireAlignedWord;
  bool fractionalCounting;

  bool sourceSyntax, targetSyntax;
  int mixedSyntaxType;

  std::string defaultNonTerm;
};

