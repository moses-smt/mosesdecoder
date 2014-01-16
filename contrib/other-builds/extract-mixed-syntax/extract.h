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
#include "Tunnel.h"
#include "TunnelCollection.h"
#include "SentenceAlignment.h"
#include "Global.h"

std::vector<std::string> tokenize( const char [] );

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

const Global *g_global;

std::ofstream extractFile;
std::ofstream extractFileInv;
std::ofstream extractFileOrientation;

std::set< std::string > targetLabelCollection, sourceLabelCollection;
std::map< std::string, int > targetTopLabelCollection, sourceTopLabelCollection;
