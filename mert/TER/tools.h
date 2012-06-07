#ifndef MERT_TER_TOOLS_H_
#define MERT_TER_TOOLS_H_

#include <vector>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>

using namespace std;

namespace Tools
{
typedef vector<double> vecDouble;
typedef vector<char> vecChar;
typedef vector<int> vecInt;
typedef vector<float> vecFloat;
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
  bool noTxtIds;
};
// param = { false, "","","","" };

// class tools{
// private:
// public:

string vectorToString ( vector<string> vec );
string vectorToString ( vector<string> vec, string s );
vector<string> subVector ( vector<string> vec, int start, int end );
vector<int> subVector ( vector<int> vec, int start, int end );
vector<float> subVector ( vector<float> vec, int start, int end );
vector<string> copyVector ( vector<string> vec );
vector<int> copyVector ( vector<int> vec );
vector<float> copyVector ( vector<float> vec );
vector<string> stringToVector ( string s, string tok );
vector<int> stringToVectorInt ( string s, string tok );
vector<float> stringToVectorFloat ( string s, string tok );
string lowerCase(string str);
string removePunct(string str);
string tokenizePunct(string str);
string removePunctTercom(string str);
string normalizeStd(string str);
string printParams(param p);
// };
param copyParam(param p);

}

#endif  // MERT_TER_TOOLS_H_
