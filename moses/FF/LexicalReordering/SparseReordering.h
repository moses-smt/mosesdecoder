#ifndef moses_FF_LexicalReordering_SparseReordering_h
#define moses_FF_LexicalReordering_SparseReordering_h

/**
 * Sparse reordering features for phrase-based MT, following Cherry (NAACL, 2013)
**/


#include <map>
#include <string>

#include "moses/ScoreComponentCollection.h"
#include "LexicalReorderingState.h"

namespace Moses
{
class SparseReordering
{
public:
  SparseReordering(const std::map<std::string,std::string>& config);
  
  void AddScores(const TranslationOption& topt,
                 LexicalReorderingState::ReorderingType reoType,
                 LexicalReorderingConfiguration::Direction direction,
                 ScoreComponentCollection* scores) const ;

};



} //namespace


#endif
