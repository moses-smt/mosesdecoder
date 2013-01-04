/***********************************************************************
Relative Entropy-based Phrase table Pruning
Copyright (C) 2012 Wang Ling

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include <vector>
#include "Hypothesis.h"
#include "StaticData.h"
#include "RelativeEntropyCalc.h"
#include "Manager.h"

using namespace std;
using namespace Moses;
using namespace MosesCmd;

namespace MosesCmd
{
  double RelativeEntropyCalc::CalcRelativeEntropy(int translationId, std::vector<SearchGraphNode>& searchGraph){
      const StaticData &staticData = StaticData::Instance();
      const Phrase *m_constraint = staticData.GetConstrainingPhrase(translationId);

      double prunedScore = -numeric_limits<double>::max();
      double unprunedScore =  -numeric_limits<double>::max();
      for (size_t i = 0; i < searchGraph.size(); ++i) {
         const SearchGraphNode& searchNode = searchGraph[i];
         int nodeId = searchNode.hypo->GetId();
         if(nodeId == 0) continue; // initial hypothesis

         int forwardId = searchNode.forward;
 	 if(forwardId == -1){ // is final hypothesis
            Phrase catOutput(0);
	    ConcatOutputPhraseRecursive(catOutput, searchNode.hypo);
	    if(catOutput == *m_constraint){ // is the output actually the same as the constraint (forced decoding does not always force the output)
               const Hypothesis *prevHypo = searchNode.hypo->GetPrevHypo();
               int backId = prevHypo->GetId();
	       double derivationScore = searchNode.hypo->GetScore();
	       if(backId != 0){ // derivation using smaller units
		  if(prunedScore < derivationScore){
		     prunedScore = derivationScore;
	          }
	       }
	       if(unprunedScore < derivationScore){
		  unprunedScore = derivationScore;
	       }
	    }
	 }
      }

      double neg_log_div = 0;
      if( unprunedScore == -numeric_limits<double>::max()){
	neg_log_div = numeric_limits<double>::max(); // could not find phrase pair, give it a low score so that it doesnt get pruned
      }
      else{
      	neg_log_div = unprunedScore - prunedScore;
      }
      if (neg_log_div > 100){
	 return 100;
      }
      return neg_log_div; 
  }

  void RelativeEntropyCalc::ConcatOutputPhraseRecursive(Phrase& phrase, const Hypothesis *hypo){
      int nodeId = hypo->GetId();
      if(nodeId == 0) return; // initial hypothesis
      ConcatOutputPhraseRecursive(phrase, hypo->GetPrevHypo());
      const Phrase &endPhrase = hypo->GetCurrTargetPhrase();
      phrase.Append(endPhrase);
  }
}
