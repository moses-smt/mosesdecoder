#pragma once
/*
 *  Global.h
 *  extract
 *
 *  Created by Hieu Hoang on 01/02/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <set>
#include <map>
#include <string>

class Global
{
public:
	int minHoleSpanSourceDefault;
	int maxHoleSpanSourceDefault;
	int minHoleSpanSourceSyntax;
	int maxHoleSpanSourceSyntax;

	int maxSymbolsSource;
	bool glueGrammarFlag;
	bool unknownWordLabelFlag;
	int maxNonTerm;
	int maxNonTermDefault;
	bool sourceSyntax;
	bool targetSyntax;
	bool mixed;
	int maxUnaligned;
	bool uppermostOnly;
	bool allowDefaultNonTermEdge;
	
	Global();

	Global(const Global&);

};

extern bool g_debug;

#define DEBUG_OUTPUT()	 void DebugOutput() const;


