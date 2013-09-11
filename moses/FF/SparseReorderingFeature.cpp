#include <iostream>

#include "moses/ChartHypothesis.h"

#include "SparseReorderingFeature.h"

using namespace std;

namespace Moses
{

SparseReorderingFeature::SparseReorderingFeature(const std::string &line)
  :StatefulFeatureFunction("StatefulFeatureFunction",0, line)
{
  cerr << "Constructing a Sparse Reordering feature" << endl;
}

FFState* SparseReorderingFeature::EvaluateChart(
  const ChartHypothesis&  cur_hypo ,
  int  featureID /*- used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  // get index map for underlying hypotheses
  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
    cur_hypo.GetCurrTargetPhrase().GetAlignNonTerm().GetNonTermIndexMap();
  
  //Find all the pairs of non-terminals
  //Are they forward or reversed relative to each other?
  //Add features for their boundary words

  //Get mapping from target to source, in target order
  vector<pair<size_t, size_t> > targetNTs; //(srcIdx,targetPos)
  for (size_t targetIdx = 0; targetIdx < nonTermIndexMap.size(); ++targetIdx) {
    size_t srcNTIdx;
    if ((srcNTIdx = nonTermIndexMap[targetIdx]) == NOT_FOUND) continue;
    targetNTs.push_back(pair<size_t,size_t> (srcNTIdx,targetIdx));
  }
  for (size_t i = 0; i < targetNTs.size(); ++i) {
    for (size_t j = i+1; j < targetNTs.size(); ++j) {
      size_t src1 = targetNTs[i].first;
      size_t src2 = targetNTs[j].first;
      //NT pair (src1,src2) maps to (i,j)
      cerr << src1 << " -> " << i << " , " << src2 << " -> " << j << endl; 
    }
  }

  return new SparseReorderingState();
}


}

