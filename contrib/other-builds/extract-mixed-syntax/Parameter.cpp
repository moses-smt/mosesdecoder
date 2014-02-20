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
,nonTermConsecSource(false)

,sourceSyntax(false)
,targetSyntax(false)
{}

Parameter::~Parameter() {
	// TODO Auto-generated destructor stub
}

