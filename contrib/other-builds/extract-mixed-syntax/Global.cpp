/*
 *  Global.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 01/02/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "Global.h"

bool g_debug = false;

Global::Global()
: minHoleSpanSourceDefault(2)
, maxHoleSpanSourceDefault(7)
, minHoleSpanSourceSyntax(1)
, maxHoleSpanSourceSyntax(1000)
, maxUnaligned(99999)

, maxSpan(10)
, maxSymbolsSource(5)
, maxSymbolsTarget(999)
, maxNonTerm(3)
, maxNonTermDefault(2)

// int minHoleSize(1)
// int minSubPhraseSize(1) // minimum size of a remaining lexical phrase 
, glueGrammarFlag(false)
, unknownWordLabelFlag(false)
//bool zipFiles(false)
, sourceSyntax(true)
, targetSyntax(false)
, mixed(true)
, uppermostOnly(true)
, allowDefaultNonTermEdge(true)
, gzOutput(false)

{}
