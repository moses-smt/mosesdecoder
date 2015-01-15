/*
 * Parameter.cpp
 *
 *  Created on: 17 Feb 2014
 *      Author: hieu
 */
#include "Parameter.h"
#include "moses/Util.h"
#include "util/exception.hh"

using namespace std;

Parameter::Parameter()
  :maxSpan(10)
  ,minSpan(0)
  ,maxNonTerm(2)
  ,maxHieroNonTerm(999)
  ,maxSymbolsTarget(999)
  ,maxSymbolsSource(5)
  ,minHoleSource(2)
  ,minHoleSourceSyntax(1)
  ,sentenceOffset(0)
  ,nonTermConsecSource(false)
  ,requireAlignedWord(true)
  ,fractionalCounting(true)
  ,gzOutput(false)

  ,hieroNonTerm("[X]")
  ,sourceSyntax(false)
  ,targetSyntax(false)

  ,mixedSyntaxType(0)
  ,multiLabel(0)
  ,nonTermConsecSourceMixed(true)
  ,hieroSourceLHS(false)
  ,maxSpanFreeNonTermSource(0)
  ,nieceTerminal(true)
  ,maxScope(UNDEFINED)
  ,minScope(0)

  ,spanLength(false)
  ,ruleLength(false)
  ,nonTermContext(false)
  ,nonTermContextTarget(false)
  ,nonTermContextFactor(0)

  ,numSourceFactors(1)
  ,numTargetFactors(1)

  ,nonTermConsecSourceMixedSyntax(1)
{}

Parameter::~Parameter()
{
  // TODO Auto-generated destructor stub
}

void Parameter::SetScopeSpan(const std::string &str)
{
  scopeSpanStr = str;
  vector<string> toks1;
  Moses::Tokenize(toks1, str, ":");

  for (size_t i = 0; i < toks1.size(); ++i) {
    const string &tok1 = toks1[i];

    vector<int> toks2;
    Moses::Tokenize<int>(toks2, tok1, ",");
    UTIL_THROW_IF2(toks2.size() != 2, "Format is min,max:min,max... String is " << tok1);

    std::pair<int,int> values(toks2[0], toks2[1]);
    scopeSpan.push_back(values);
  }
}
