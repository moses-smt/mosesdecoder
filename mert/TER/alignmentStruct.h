#ifndef MERT_TER_ALIGNMENT_STRUCT_H_
#define MERT_TER_ALIGNMENT_STRUCT_H_

#include <vector>
#include <stdio.h>
#include <string>
#include <sstream>
#include "tools.h"

using namespace std;
using namespace Tools;

namespace TERCpp
{
class alignmentStruct
{
private:
public:

// 	alignmentStruct();
// 	alignmentStruct (int _start, int _end, int _moveto, int _newloc);
// 	alignmentStruct (int _start, int _end, int _moveto, int _newloc, vector<string> _shifted);
// 	string toString();
// 	int distance() ;
// 	bool leftShift();
// 	int size();
// 	alignmentStruct operator=(alignmentStruct t);
// 	string vectorToString(vector<string> vec);

//   int start;
//   int end;
//   int moveto;
//   int newloc;
  vector<string> nwords; // The words we shifted
  vector<char> alignment ; // for pra_more output
  vector<vecInt> aftershift; // for pra_more output
  // This is used to store the cost of a shift, so we don't have to
  // calculate it multiple times.
  double cost;
  string toString();
};

}

#endif  // MERT_TER_ALIGNMENT_STRUCT_H_
