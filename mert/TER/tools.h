/*********************************
tercpp: an open-source Translation Edit Rate (TER) scorer tool for Machine Translation.

Copyright 2010-2013, Christophe Servan, LIUM, University of Le Mans, France
Contact: christophe.servan@lium.univ-lemans.fr

The tercpp tool and library are free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the licence, or
(at your option) any later version.

This program and library are distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
**********************************/
#ifndef __TERCPPTOOLS_H__
#define __TERCPPTOOLS_H__


#include <vector>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sstream>
#include <boost/xpressive/xpressive.hpp>


using namespace std;

namespace TERCPPNS_Tools
{
typedef vector<double> vecDouble;
typedef vector<char> vecChar;
typedef vector<int> vecInt;
typedef vector<float> vecFloat;
typedef vector<size_t> vecSize_t;
typedef vector<string> vecString;
typedef vector<string> alignmentElement;
typedef vector<alignmentElement> WERalignment;


struct param {
  bool debugMode;
  string referenceFile;     // path to the resources
  string hypothesisFile;     // path to the configuration files
  string outputFileExtension;
  string outputFileName;
  bool noPunct;
  bool caseOn;
  bool normalize;
  bool tercomLike;
  bool sgmlInputs;
  bool verbose;
  bool count_verbose;
  bool noTxtIds;
  bool printAlignments;
  bool WER;
  int debugLevel;
};
// param = { false, "","","","" };

// class tools{
// private:
// public:

string vectorToString ( vector<string> vec );
string vectorToString ( vector<char> vec );
string vectorToString ( vector<int> vec );
string vectorToString ( vector<string> vec, string s );
string vectorToString ( vector<char> vec, string s );
string vectorToString ( vector<int> vec, string s );
string vectorToString ( vector<bool> vec, string s );
string vectorToString ( char* vec, string s, int taille );
string vectorToString ( int* vec, string s , int taille );
string vectorToString ( bool* vec, string s , int taille );
string vectorToString ( vector<char>* vec, string s, int taille );
string vectorToString ( vector<int>* vec, string s , int taille );
string vectorToString ( vector<bool>* vec, string s , int taille );
vector<string> subVector ( vector<string> vec, int start, int end );
vector<int> subVector ( vector<int> vec, int start, int end );
vector<float> subVector ( vector<float> vec, int start, int end );
vector<string> copyVector ( vector<string> vec );
vector<int> copyVector ( vector<int> vec );
vector<float> copyVector ( vector<float> vec );
vector<string> stringToVector ( string s, string tok );
vector<string> stringToVector ( char s, string tok );
vector<string> stringToVector ( int s, string tok );
vector<int> stringToVectorInt ( string s, string tok );
vector<float> stringToVectorFloat ( string s, string tok );
string lowerCase(string str);
string removePunct(string str);
string tokenizePunct(string str);
string removePunctTercom(string str);
string normalizeStd(string str);
string printParams(param p);
string join ( string delim, vector<string> arr );
// };
param copyParam(param p);
}
#endif
