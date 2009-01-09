#pragma once

#include <string>
#include <vector>
#include "Factor.h"
#include "Phrase.h"
#include "TypeDef.h"
#include "Util.h"
#include "WordsRange.h"
#include "ScoreProducer.h"

class Factor;
class Phrase;
class Hypothesis;
class InputType;

class LexicalDistortionCost : public ScoreProducer {
 public: //types & consts
  enum Direction {Forward, Backward, Bidirectional, Unidirectional = Backward};
  enum Condition {PhrasePair, Word, SourcePhrase};
 public: //con- & destructors 
  LexicalDistortionCost(const std::string &filePath, 
		    Direction direction, 
		    Condition condition, 
		    std::vector< FactorType >& f_factors, 
		    std::vector< FactorType >& e_factors);
  virtual ~LexicalDistortionCost();
 public: //interface
  //inherited
  virtual size_t GetNumScoreComponents() const {
    return GetNumParameterSets();
  };
  
  virtual std::string GetScoreProducerDescription() const {
    return "Generic Lexical Distortion Cost model... overwrite in subclass.";
  };
  //new 

  // number of parameters per direction
  virtual size_t          GetNumParameters() const = 0;

  size_t GetNumParameterSets() const {
    return m_direction == Bidirectional ? 2 : 1;
  }

  //- virtual int             GetNumOrientationTypes() const = 0;
  //- virtual OrientationType GetOrientationType(Hypothesis*) const = 0;

  std::vector<float> CalcScore(Hypothesis* hypothesis) const;

  //- Score GetProb(const Phrase& f, const Phrase& e) const;
  //helpers
  //- static std::vector<Condition> DecodeCondition(Condition c);
  //- static std::vector<Direction> DecodeDirection(Direction d);

 protected:
  virtual float CalculateDistortionScore(	const WordsRange &prev,
						const WordsRange &curr,
						const std::vector<float> *parameters) const = 0;
 private:
   const std::vector<float> *GetDistortionParameters(std::string key, Direction dir) const;
   bool LoadTable(std::string fileName);

   typedef std::map< std::string, const std::vector<float>* > _DistortionTableType;
   Direction m_direction;
   Condition m_condition;
   std::vector<FactorType> m_srcfactors, m_tgtfactors;
   _DistortionTableType m_distortionTableForward, m_distortionTableBackward;
   std::vector<float> m_defaultDistortion;

  //- size_t m_NumScoreComponents;
};

class LDCBetaBinomial : public LexicalDistortionCost {
 public:
  LDCBetaBinomial(const std::string &filePath, 
		    Direction direction, 
		    Condition condition, 
		    std::vector< FactorType >& f_factors, 
		    std::vector< FactorType >& e_factors)
    : LexicalDistortionCost(filePath, direction, condition, f_factors, e_factors),
      m_distortionRange(6) {
      UserMessage::Add("Created beta-binomial lexical distortion cost model");
  }
  virtual std::string GetScoreProducerDescription() const {
    return "Beta-Binomial lexical distortion cost model";
  };
  virtual size_t GetNumParameters() const {
    return 2;
  }

 protected:
  virtual float CalculateDistortionScore(	const WordsRange &prev,
						const WordsRange &curr,
						const std::vector<float> *parameters) const;

 private:
  float beta_binomial(float p, float q, int x) const;
  int m_distortionRange;
};
