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
 protected: //con- & destructors 
  LexicalDistortionCost(const std::string &filePath, 
				Direction direction, 
				Condition condition, 
				std::vector< FactorType >& f_factors, 
				std::vector< FactorType >& e_factors,
				size_t numParametersPerDirection);
 public:
  virtual ~LexicalDistortionCost();
 public: //interface
  static LexicalDistortionCost *CreateModel(const std::string &modelType,
				const std::string &filePath, 
				Direction direction,
				std::vector< FactorType >& f_factors, 
				std::vector< FactorType >& e_factors);

  virtual size_t GetNumScoreComponents() const {
    return GetNumParameterSets();
  };
  
  virtual std::string GetScoreProducerDescription() const = 0;

  // number of parameters per direction
  size_t GetNumParametersPerDirection() const {
    return m_numParametersPerDirection;
  }

  size_t GetNumParameterSets() const {
    return m_direction == Bidirectional ? 2 : 1;
  }

  std::vector<float> CalcScore(Hypothesis* hypothesis) const;

  std::vector<float> GetProb(const Phrase &src, const Phrase &tgt) const;

 protected:
   virtual float CalculateDistortionScore(	const WordsRange &prev,
						const WordsRange &curr,
						const std::vector<float> *parameters) const = 0;

   virtual std::vector<float> GetDefaultDistortion() const = 0;

   std::string m_modelFileName;

 private:
   const std::vector<float> GetDistortionParameters(const Phrase &src, const Phrase &tgt) const;

   LexicalReorderingTable *m_distortionTable;
   Direction m_direction;
   Condition m_condition;
   std::vector<FactorType> m_srcfactors, m_tgtfactors;
   std::vector<float> m_defaultDistortion;
   size_t m_numParametersPerDirection;
};

class LDCBetaBinomial : public LexicalDistortionCost {
  friend class LexicalDistortionCost;

 protected:
  LDCBetaBinomial(const std::string &filePath, 
		    Direction direction, 
		    Condition condition, 
		    std::vector< FactorType >& f_factors, 
		    std::vector< FactorType >& e_factors)
    : LexicalDistortionCost(filePath, direction, condition, f_factors, e_factors, 2),
      m_distortionRange(6) {
       VERBOSE(1, "Created beta-binomial lexical distortion cost model\n");
  }

 public:
  virtual std::string GetScoreProducerDescription() const {
    return "Beta-Binomial lexical distortion cost model, file=" + m_modelFileName;
  };
  virtual size_t GetNumParameters() const {
    return 2;
  }

 protected:
  virtual float CalculateDistortionScore(	const WordsRange &prev,
						const WordsRange &curr,
						const std::vector<float> *parameters) const;

  virtual std::vector<float> GetDefaultDistortion() const;

 private:
  float beta_binomial(float p, float q, int x) const;
  int m_distortionRange;
};

class LDCBetaGeometric : public LexicalDistortionCost {
  friend class LexicalDistortionCost;

 protected:
  LDCBetaGeometric(const std::string &filePath, 
		    Direction direction, 
		    Condition condition, 
		    std::vector< FactorType >& f_factors, 
		    std::vector< FactorType >& e_factors)
    : LexicalDistortionCost(filePath, direction, condition, f_factors, e_factors, 6) {
       VERBOSE(1, "Created beta-geometric lexical distortion cost model\n");
  }

 public:
  virtual std::string GetScoreProducerDescription() const {
    return "Beta-Geometric lexical distortion cost model, file=" + m_modelFileName;
  };
  virtual size_t GetNumParameters() const {
    return 6;
  }

 protected:
  virtual float CalculateDistortionScore(	const WordsRange &prev,
						const WordsRange &curr,
						const std::vector<float> *parameters) const;

  virtual std::vector<float> GetDefaultDistortion() const;

 private:
  float beta_geometric(float p, float q, int x) const;
};
