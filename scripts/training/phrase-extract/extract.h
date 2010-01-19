#pragma once

#include <vector>
#include <list>
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "SyntaxTree.h"
#include "XmlTree.h"
#include "Hole.h"
#include "HoleCollection.h"
#include "RuleExist.h"
#include "SentenceAlignment.h"

typedef std::map< int, int > WordIndex;

std::vector< ExtractedRule > extractedRules;
void extractRules( SentenceAlignment & );
void addRuleToCollection( ExtractedRule &rule );
void consolidateRules();
void writeRulesToFile();
void writeGlueGrammar( std::string );
void collectWordLabelCounts( SentenceAlignment &sentence );
void writeUnknownWordLabel( std::string );

typedef std::vector< int > LabelIndex;

// functions
void openFiles();
void addRule( SentenceAlignment &, int, int, int, int 
							, RuleExist &ruleExist);
void addHieroRule( SentenceAlignment &sentence, int startT, int endT, int startS, int endS 
									 , RuleExist &ruleExist, const HoleCollection &holeColl, int numHoles, int initStartF, int wordCountT, int wordCountS);

std::vector<std::string> tokenize( const char [] );
bool isAligned ( SentenceAlignment &, int, int );

std::ofstream extractFile;
std::ofstream extractFileInv;
std::ofstream extractFileOrientation;
int maxSpan = 10;
int minHoleSource = 2;
int minHoleTarget = 1;
int minWords = 1;
int maxSymbolsTarget = 999;
int maxSymbolsSource = 5;
// int minHoleSize = 1;
// int minSubPhraseSize = 1; // minimum size of a remaining lexical phrase 
int phraseCount = 0;
bool onlyDirectFlag = false;
bool orientationFlag = false;
bool glueGrammarFlag = false;
bool unknownWordLabelFlag = false;
std::set< std::string > targetLabelCollection, sourceLabelCollection;
std::map< std::string, int > targetTopLabelCollection, sourceTopLabelCollection;
bool hierarchicalFlag = false;
bool onlyOutputSpanInfo = false;
bool noFileLimit = false;
//bool zipFiles = false;
bool properConditioning = false;
int maxNonTerm = 2;
bool nonTermFirstWord = true;
bool nonTermConsecTarget = true;
bool nonTermConsecSource = false;
bool requireAlignedWord = true;
bool sourceSyntax = false;
bool targetSyntax = false;
bool duplicateRules = true;
bool fractionalCounting = true;

#define SAFE_GETLINE(_IS, _LINE, _SIZE, _DELIM) { \
                _IS.getline(_LINE, _SIZE, _DELIM); \
                if(_IS.fail() && !_IS.bad() && !_IS.eof()) _IS.clear(); \
                if (_IS.gcount() == _SIZE-1) { \
                  cerr << "Line too long! Buffer overflow. Delete lines >=" \
                    << _SIZE << " chars or raise LINE_MAX_LENGTH in phrase-extract/extract.cpp" \
                    << endl; \
                    exit(1); \
                } \
              }
#define LINE_MAX_LENGTH 1000000

std::string IntToString( int i )
{
	std::string s;
	std::stringstream out;
	out << i;
	return out.str();
}

