#pragma once

#include "FeatureFunction.h"
#include "moses/HypothesisStackNormal.h"
#include "moses/TrellisPathList.h"
#include "moses/Syntax/SHyperedge.h"

namespace Moses
{

/** base class for all Asynchronous feature functions.
 * eg. Neural Network Language model rescoring (CSLM)
 */
class AsynchFeatureFunction: public FeatureFunction
{
  //All stateless FFs, except those that cache scores in T-Option
  static std::vector<const AsynchFeatureFunction*> m_AsynchFFs; 

public:
  static const std::vector<const AsynchFeatureFunction*>& GetAsynchFeatureFunctions() {
    return m_AsynchFFs; //m_statelessFFs;
  }

  AsynchFeatureFunction(const std::string &line);
  AsynchFeatureFunction(size_t numScoreComponents, const std::string &line);


   /*
    * Function to evaluate Nbest after first pass-decoding 
    */
 
 virtual void EvaluateNbest(const InputType &input,
		 		    const TrellisPathList &Nbest) const = 0;

  /*
   * This should be implemented for Asynch features applied to Search Graph (a.k.a stacks used to store partial translations )
   */
// virtual void EvaluateSearchGraph(const InputType &input,
//				  const std::vector < HypothesisStack* > &hypoStackColl );

  virtual bool IsStateless() const {
    return true;
  }

};


} // namespace

