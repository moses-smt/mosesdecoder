//#ifndef MAXENTREORDERING_H_
//#define MAXENTREORDERING_H_
#pragma once

#include <string>
#include <vector>
#include "Factor.h"
#include "Phrase.h"
#include "TypeDef.h"
#include "Util.h"
#include "WordsRange.h"
#include "ScoreProducer.h"

#include "MaxentReorderingTable.h"

namespace Moses
{

class Factor;
class Phrase;
class Hypothesis;
class InputType;

using namespace std;

class MaxentReordering : public ScoreProducer {
 public: //types & consts
  typedef int OrientationType; 
  enum Direction {Forward, Backward, Bidirectional, Unidirectional = Backward};
  enum Condition {F,E,C,FE,FEC};
 public: //con- & destructors 
  MaxentReordering(const std::string &filePath, 
		    const std::vector<float>& weights, 
		    Condition condition, 
		    std::vector< FactorType >& f_factors, 
		    std::vector< FactorType >& e_factors);
  virtual ~MaxentReordering();
 public: //interface
  //inherited
  virtual size_t GetNumScoreComponents() const {
    return m_NumScoreComponents; 
  };
  
  virtual std::string GetScoreProducerDescription() const {
    return "Generic Maxent Reordering Model... overwrite in subclass.";
  };
  //new 
  virtual int             GetNumOrientationTypes() const = 0;
  virtual vector<OrientationType> GetOrientationType(Hypothesis*) const = 0;
//  virtual OrientationType GetOrientationType(Hypothesis*) const = 0;
  
  std::vector<float> CalcScore(Hypothesis* hypothesis) const;
  void InitializeForInput(const InputType& i){
    m_Table->InitializeForInput(i);
  }

	Score GetProb(const Phrase& f, const Phrase& e, const Phrase& f_context) const;
	
  //helpers
  static std::vector<Condition> DecodeCondition(Condition c);
  static std::vector<Direction> DecodeDirection(Direction d);
 private:
  Phrase auxGetContext(const Hypothesis* hypothesis) const;
 private:
  MaxentReorderingTable* m_Table;
  size_t m_NumScoreComponents;
  std::vector< Direction > m_Direction;
  std::vector< Condition > m_Condition;
  std::vector< FactorType > m_FactorsE, m_FactorsF, m_FactorsC;
  int m_MaxContextLength;
};


class MaxentOrientationReordering : public MaxentReordering {
 private:
  enum {RIGHT = 0, RIGHT_PLUS = 1, LEFT = 2, LEFT_PLUS = 3, LEFT_undef = 4, NONE = 5};
 public:
    MaxentOrientationReordering(const std::string &filePath, 
			     const std::vector<float>& w, 
			     Condition condition, 
			     std::vector< FactorType >& f_factors, 
			     std::vector< FactorType >& e_factors)
			: MaxentReordering(filePath, w, condition, f_factors, e_factors){
	  std::cerr << "Created maxent orientation reordering\n";
  }
 public:
  virtual int GetNumOrientationTypes() const {
//    return 3;
		// give number of maxent outcomes
		return 4; 
  }
  virtual std::string GetScoreProducerDescription() const {
    return "OrientationMaxentReorderingModel";
  };
  virtual std::vector<OrientationType> GetOrientationType(Hypothesis* currHypothesis) const;
//  virtual OrientationType GetOrientationType(Hypothesis* currHypothesis) const;
  MaxentReordering::OrientationType ReEvaluateWithNextPhraseInSource(Hypothesis* currHypothesis, const WordsRange currSourceWordsRange) const;
};

}
//#endif /*MAXENTREORDERING_H_*/

