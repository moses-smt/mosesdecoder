#ifndef MERT_TER_TER_ALIGNMENT_H_
#define MERT_TER_TER_ALIGNMENT_H_

#include <vector>
#include <stdio.h>
#include <string.h>
#include "tools.h"
#include "terShift.h"


using namespace std;
// using namespace HashMapSpace;
namespace TERCpp
{

class terAlignment
{
private:
public:

  terAlignment();
  string toString();
  void scoreDetails();

  vector<string> ref;
  vector<string> hyp;
  vector<string> aftershift;

  vector<terShift> allshifts;

  double numEdits;
  double numWords;
  double averageWords;
  vector<char> alignment;
  string bestRef;

  int numIns;
  int numDel;
  int numSub;
  int numSft;
  int numWsf;


  string join ( string delim, vector<string> arr );
  double score();
  double scoreAv();
};

}

#endif  // MERT_TER_TER_ALIGNMENT_H__
