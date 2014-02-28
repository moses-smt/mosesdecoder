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

  long sentenceOffset;

  bool nonTermConsecSource;
  bool requireAlignedWord;
  bool fractionalCounting;
  bool gzOutput;

  std::string hieroNonTerm;
  std::string gluePath;

  bool sourceSyntax, targetSyntax;

  int mixedSyntaxType;
  bool nonTermConsecSourceMixed;

};

