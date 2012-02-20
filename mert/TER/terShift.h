#ifndef MERT_TER_TER_SHIFT_H_
#define MERT_TER_TER_SHIFT_H_

#include <vector>
#include <stdio.h>
#include <string>
#include <sstream>
#include "tools.h"

using namespace std;
using namespace Tools;

namespace TERCpp
{
class terShift
{
private:
public:

  terShift();
  terShift ( int _start, int _end, int _moveto, int _newloc );
  terShift ( int _start, int _end, int _moveto, int _newloc, vector<string> _shifted );
  string toString();
  int distance() ;
  bool leftShift();
  int size();
// 	terShift operator=(terShift t);
// 	string vectorToString(vector<string> vec);

  int start;
  int end;
  int moveto;
  int newloc;
  vector<string> shifted; // The words we shifted
  vector<char> alignment ; // for pra_more output
  vector<string> aftershift; // for pra_more output
  // This is used to store the cost of a shift, so we don't have to
  // calculate it multiple times.
  double cost;
};

}

#endif  // MERT_TER_TER_SHIFT_H_
