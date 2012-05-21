#ifndef MERT_TER_TER_CALC_H_
#define MERT_TER_TER_CALC_H_

#include <vector>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include "hashMap.h"
#include "hashMapInfos.h"
#include "hashMapStringInfos.h"
#include "terAlignment.h"
#include "tools.h"
#include "terShift.h"
#include "alignmentStruct.h"
#include "bestShiftStruct.h"

using namespace std;
using namespace Tools;
using namespace HashMapSpace;
namespace TERCpp
{
// typedef size_t WERelement[2];
// Vecteur d'alignement contenant le hash du mot et son evaluation (0=ok, 1=sub, 2=ins, 3=del)
typedef vector<terShift> vecTerShift;
/**
	@author
*/
class terCalc
{
private :
// Vecteur d'alignement contenant le hash du mot et son evaluation (0=ok, 1=sub, 2=ins, 3=del)
  WERalignment l_WERalignment;
// HashMap contenant les caleurs de hash de chaque mot
  hashMap bagOfWords;
  int MAX_SHIFT_SIZE;
  /* Variables for some internal counting.  */
  int NUM_SEGMENTS_SCORED;
  int NUM_SHIFTS_CONSIDERED;
  int NUM_BEAM_SEARCH_CALLS;
  int MAX_SHIFT_DIST;
  bool PRINT_DEBUG;

  /* These are resized by the MIN_EDIT_DIST code if they aren't big enough */
  double S[1000][1000];
  char P[1000][1000];
  vector<vecInt> refSpans;
  vector<vecInt> hypSpans;
  int BEAM_WIDTH;

public:
  int shift_cost;
  int insert_cost;
  int delete_cost;
  int substitute_cost;
  int match_cost;
  double INF;
  terCalc();

//     ~terCalc();
//             size_t* hashVec ( vector<string> s );
  void setDebugMode ( bool b );
  int WERCalculation ( size_t * ref, size_t * hyp );
  int WERCalculation ( vector<string> ref, vector<string> hyp );
  int WERCalculation ( vector<int> ref, vector<int> hyp );
// 	string vectorToString(vector<string> vec);
// 	vector<string> subVector(vector<string> vec, int start, int end);
  hashMapInfos BuildWordMatches ( vector<string> hyp, vector<string> ref );
  terAlignment MinEditDist ( vector<string> hyp, vector<string> ref, vector<vecInt> curHypSpans );
  bool spanIntersection ( vecInt refSpan, vecInt hypSpan );
  terAlignment TER ( vector<string> hyp, vector<string> ref , float avRefLength );
  terAlignment TER ( vector<string> hyp, vector<string> ref );
  terAlignment TER ( vector<int> hyp, vector<int> ref );
  bestShiftStruct CalcBestShift ( vector<string> cur, vector<string> hyp, vector<string> ref, hashMapInfos rloc, terAlignment cur_align );
  void FindAlignErr ( terAlignment align, bool* herr, bool* rerr, int* ralign );
  vector<vecTerShift> GatherAllPossShifts ( vector<string> hyp, vector<string> ref, hashMapInfos rloc, terAlignment align, bool* herr, bool* rerr, int* ralign );
  alignmentStruct PerformShift ( vector<string> words, terShift s );
  alignmentStruct PerformShift ( vector<string> words, int start, int end, int newloc );
};

}

#endif  // MERT_TER_TER_CALC_H_
