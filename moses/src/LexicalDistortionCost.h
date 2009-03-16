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

  size_t GetNumPriorParameters() const {
    // only one set of parameters, no matter how many directions!
    return GetNumParametersPerDirection();
  }

  // number of parameters per direction
  size_t GetNumParametersPerDirection() const {
    return m_numParametersPerDirection;
  }

  size_t GetNumParameterSets() const {
    return m_direction == Bidirectional ? 2 : 1;
  }

  std::vector<float> CalcScore(Hypothesis* hypothesis) const;

  std::vector<float> GetProb(const Phrase &src, const Phrase &tgt) const;

  std::vector<float> GetPrior() const {
    if(m_prior.size() > 0)
      return m_prior;
    else
      return GetDefaultPrior();
  }

  void SetPrior(std::vector<float> prior) {
    assert(prior.size() == GetNumParametersPerDirection());
    m_prior = prior;
  }

 protected:
   virtual float CalculateDistortionScore(	const WordsRange &prev,
						const WordsRange &curr,
						const std::vector<float> *parameters) const = 0;

   virtual std::vector<float> GetDefaultPrior() const = 0;

   std::string m_modelFileName;

   std::vector<float> m_prior;
 private:
   const std::vector<float> GetDistortionParameters(const Phrase &src, const Phrase &tgt) const;

   LexicalReorderingTable *m_distortionTable;
   Direction m_direction;
   Condition m_condition;
   std::vector<FactorType> m_srcfactors, m_tgtfactors;
   size_t m_numParametersPerDirection;
};

class LDCBetaBinomial : public LexicalDistortionCost {
  friend class LexicalDistortionCost;

 protected:
  static const size_t NUM_PARAMETERS = 2;

  LDCBetaBinomial(const std::string &filePath, 
		    Direction direction, 
		    Condition condition, 
		    std::vector< FactorType >& f_factors, 
		    std::vector< FactorType >& e_factors)
    : LexicalDistortionCost(filePath, direction, condition, f_factors, e_factors, NUM_PARAMETERS),
      m_distortionRange(6) {
       VERBOSE(1, "Created beta-binomial lexical distortion cost model\n");
  }

 public:
  virtual std::string GetScoreProducerDescription() const {
    return "Beta-Binomial lexical distortion cost model, file=" + m_modelFileName;
  };
 protected:
  virtual float CalculateDistortionScore(	const WordsRange &prev,
						const WordsRange &curr,
						const std::vector<float> *parameters) const;

  virtual std::vector<float> GetDefaultPrior() const;
 private:
  float beta_binomial(float p, float q, int x) const;
  int m_distortionRange;
};

class LDCBetaGeometric : public LexicalDistortionCost {
  friend class LexicalDistortionCost;

 protected:
  static const size_t NUM_PARAMETERS = 6;

  LDCBetaGeometric(const std::string &filePath, 
		    Direction direction, 
		    Condition condition, 
		    std::vector< FactorType >& f_factors, 
		    std::vector< FactorType >& e_factors,
                    size_t numParameters = NUM_PARAMETERS)
    : LexicalDistortionCost(filePath, direction, condition, f_factors, e_factors, numParameters) {
       VERBOSE(1, "Created beta-geometric lexical distortion cost model or subclass\n");
  }

 public:
  virtual std::string GetScoreProducerDescription() const {
    return "Beta-Geometric lexical distortion cost model, file=" + m_modelFileName;
  };

 protected:
  virtual float CalculateDistortionScore(	const WordsRange &prev,
						const WordsRange &curr,
						const std::vector<float> *parameters) const;

  virtual std::vector<float> GetDefaultPrior() const;

  float beta_geometric(float p, float q, int x) const;
};

class LDC3BetaGeometric : public LDCBetaGeometric {
  friend class LexicalDistortionCost;

 protected:
  static const size_t NUM_PARAMETERS = 7;

  LDC3BetaGeometric(const std::string &filePath, 
		    Direction direction, 
		    Condition condition, 
		    std::vector< FactorType >& f_factors, 
		    std::vector< FactorType >& e_factors)
    : LDCBetaGeometric(filePath, direction, condition, f_factors, e_factors, NUM_PARAMETERS) {
       VERBOSE(1, "Created 3-way beta-geometric lexical distortion cost model\n");
  }

 public:
  virtual std::string GetScoreProducerDescription() const {
    return "3-way Beta-Geometric lexical distortion cost model, file=" + m_modelFileName;
  };

 protected:
  virtual float CalculateDistortionScore(	const WordsRange &prev,
						const WordsRange &curr,
						const std::vector<float> *parameters) const;

  virtual std::vector<float> GetDefaultPrior() const;
};

class LDCDirichletMultinomial : public LexicalDistortionCost {
  friend class LexicalDistortionCost;

 protected:
  static const size_t NUM_PARAMETERS = 23;

  LDCDirichletMultinomial(const std::string &filePath, 
		    Direction direction, 
		    Condition condition, 
		    std::vector< FactorType >& f_factors, 
		    std::vector< FactorType >& e_factors)
    : LexicalDistortionCost(filePath, direction, condition, f_factors, e_factors, NUM_PARAMETERS) {
       VERBOSE(1, "Created Dirichlet-Multinomial lexical distortion cost model\n");
  }

 public:
  virtual std::string GetScoreProducerDescription() const {
    return "Dirichlet-Multinomial lexical distortion cost model, file=" + m_modelFileName;
  };
 protected:
  virtual float CalculateDistortionScore(	const WordsRange &prev,
						const WordsRange &curr,
						const std::vector<float> *parameters) const;

  virtual std::vector<float> GetDefaultPrior() const;
};
