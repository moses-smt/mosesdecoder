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
		int maxSpan;
		int minHoleSource;
		int minHoleTarget;
		int minWords;
		int maxSymbolsTarget;
		int maxSymbolsSource;
		bool onlyDirectFlag;
		bool orientationFlag;
		bool glueGrammarFlag;
		bool unknownWordLabelFlag;
		bool hierarchicalFlag;
		bool onlyOutputSpanInfo;
		bool noFileLimit;
		bool properConditioning;
		int maxNonTerm;
		bool nonTermFirstWord;
		bool nonTermConsecTarget;
		bool nonTermConsecSource;
		bool requireAlignedWord;
		bool sourceSyntax;
		bool targetSyntax;
		bool duplicateRules;
		bool fractionalCounting;
		bool mixed;
		
		Global()
		: maxSpan(10)
		, minHoleSource(2)
		, minHoleTarget(1)
		, minWords(1)
		, maxSymbolsTarget(999)
		, maxSymbolsSource(5)
		// int minHoleSize(1)
		// int minSubPhraseSize(1) // minimum size of a remaining lexical phrase 
		, onlyDirectFlag(false)
		, orientationFlag(false)
		, glueGrammarFlag(false)
		, unknownWordLabelFlag(false)
		, hierarchicalFlag(false)
		, onlyOutputSpanInfo(false)
		, noFileLimit(false)
		//bool zipFiles(false)
		, properConditioning(false)
		, maxNonTerm(2)
		, nonTermFirstWord(true)
		, nonTermConsecTarget(true)
		, nonTermConsecSource(false)
		, requireAlignedWord(true)
		, sourceSyntax(false)
		, targetSyntax(false)
		, duplicateRules(true)
		, fractionalCounting(true)
		, mixed(false)
		{}
		
	};

