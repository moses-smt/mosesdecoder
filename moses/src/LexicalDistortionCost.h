#pragma once

#include <string>
#include <vector>
#include "Factor.h"
#include "Phrase.h"
#include "TypeDef.h"
#include "Util.h"
#include "WordsRange.h"
#include "ScoreProducer.h"

#include "LexicalReorderingTable.h"

class Factor;
class Phrase;
class Hypothesis;
class InputType;

using namespace std;

class LexicalDistortionCost : public ScoreProducer {
 public: //types & consts
  enum Direction {Forward, Backward, Bidirectional, Unidirectional = Backward};
  enum Condition {PhrasePair, Word, SourcePhrase};
 public: //con- & destructors 
  LexicalDistortionCost(const std::string &filePath, 
		    const std::vector<float>& weights, 
		    Direction direction, 
		    Condition condition, 
		    std::vector< FactorType >& f_factors, 
		    std::vector< FactorType >& e_factors);
  virtual ~LexicalReordering();
 public: //interface
  //inherited
  virtual size_t GetNumScoreComponents() const {
    return m_NumScoreComponents; 
  };
  
  virtual std::string GetScoreProducerDescription() const {
    return "Generic Lexical Distortion Cost model... overwrite in subclass.";
  };
  //new 

  // number of parameters per direction
  virtual size_t          GetNumParameters() const = 0;

  virtual int             GetNumOrientationTypes() const = 0;
  virtual OrientationType GetOrientationType(Hypothesis*) const = 0;

  std::vector<float> CalcScore(Hypothesis* hypothesis) const;

  void InitializeForInput(const InputType& i){
    m_Table->InitializeForInput(i);
  }

	Score GetProb(const Phrase& f, const Phrase& e) const;
  //helpers
  static std::vector<Condition> DecodeCondition(Condition c);
  static std::vector<Direction> DecodeDirection(Direction d);

 protected:
  virtual float CalculateDistortionScore(	const WordsRange &prev,
						const WordsRange &curr,
						const std::vector<float> parameters);
  
 private:
  LexicalReorderingTable* m_Table;
  size_t m_NumScoreComponents;
  std::vector< Direction > m_Direction;
  std::vector< Condition > m_Condition;
  std::vector< FactorType > m_FactorsE, m_FactorsF;
};

class LDCBetaBinomial : public LexicalDistortionCost {
public:
  LexicalDistortionCost(const std::string &filePath, 
		    const std::vector<float>& weights, 
		    Direction direction, 
		    Condition condition, 
		    std::vector< FactorType >& f_factors, 
		    std::vector< FactorType >& e_factors)
    : LexicalDistortionCost(filePath, weights, direction, condition, f_factors, e_factors) {
      UserMessage::Add("Created beta-binomial lexical distortion cost model");
  }
  virtual std::string GetScoreProducerDescription() const {
    return "Beta-Binomial lexical distortion cost model";
  };
  virtual size_t GetNumParameters() const {
    return 2;
  }

 private:
  float beta_binomial(float p, float q, int x) const;
}
