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
#include "Global.h"

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

inline std::string IntToString( int i )
{
	std::string s;
	std::stringstream out;
	out << i;
	return out.str();
}

const Global *g_global;

int phraseCount;
std::ofstream extractFile;
std::ofstream extractFileInv;
std::ofstream extractFileOrientation;
std::set< std::string > targetLabelCollection, sourceLabelCollection;
std::map< std::string, int > targetTopLabelCollection, sourceTopLabelCollection;
