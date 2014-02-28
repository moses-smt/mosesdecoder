/*
 * Parameter.cpp
 *
 *  Created on: 17 Feb 2014
 *      Author: hieu
 */
#include "Parameter.h"

Parameter::Parameter()
:maxSpan(10)
,maxNonTerm(2)
,maxSymbolsTarget(999)
,maxSymbolsSource(5)
,minHoleSource(2)
,sentenceOffset(0)
,nonTermConsecSource(false)
,requireAlignedWord(true)
,fractionalCounting(true)
,gzOutput(false)

,sourceSyntax(false)
,targetSyntax(false)
,mixedSyntaxType(0)

,hieroNonTerm("[X]")
{}

Parameter::~Parameter() {
	// TODO Auto-generated destructor stub
}

