//#ifndef LEXICAL_REORDERING_H
//#define LEXICAL_REORDERING_H

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

class LexicalReordering : public ScoreProducer {
 public: //types & consts
  typedef int OrientationType; 
  enum Direction {Forward, Backward, Bidirectional, Unidirectional = Backward};
  enum Condition {F,E,C,FE,FEC};
 public: //con- & destructors 
  LexicalReordering(const std::string &filePath, 
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
    return "Generic Lexical Reordering Model... overwrite in subclass.";
  };
  //new 
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
 private:
  Phrase auxGetContext(const Hypothesis* hypothesis) const;
 private:
  LexicalReorderingTable* m_Table;
  size_t m_NumScoreComponents;
  std::vector< Direction > m_Direction;
  std::vector< Condition > m_Condition;
  bool m_OneScorePerDirection;
  std::vector< FactorType > m_FactorsE, m_FactorsF, m_FactorsC;
  int m_MaxContextLength;
};


class LexicalMonotonicReordering : public LexicalReordering {
 private:
  enum {Monotone = 0, NonMonotone = 1};
 public:
  LexicalMonotonicReordering(const std::string &filePath, 
			     const std::vector<float>& w, 
			     Direction direction, 
			     Condition condition, 
			     std::vector< FactorType >& f_factors, 
			     std::vector< FactorType >& e_factors)
    : LexicalReordering(filePath, w, direction, condition, f_factors, e_factors){
	std::cerr << "Created lexical monotonic reordering\n";
  }
 public:
  virtual int GetNumOrientationTypes() const {
    return 2; 
  };
  virtual std::string GetScoreProducerDescription() const {
    return "Monotonic Lexical Reordering Model";
  };
  virtual int GetOrientationType(Hypothesis* currHypothesis) const;
};

class LexicalOrientationReordering : public LexicalReordering {
 private:
  enum {Monotone = 0, Swap = 1, Discontinuous = 2};
 public:
    LexicalOrientationReordering(const std::string &filePath, 
			     const std::vector<float>& w, 
			     Direction direction, 
			     Condition condition, 
			     std::vector< FactorType >& f_factors, 
			     std::vector< FactorType >& e_factors)
      : LexicalReordering(filePath, w, direction, condition, f_factors, e_factors){
	  std::cerr << "Created lexical orientation reordering\n";
  }
 public:
  virtual int GetNumOrientationTypes() const {
    return 3; 
  }
  virtual std::string GetScoreProducerDescription() const {
    return "Orientation Lexical Reordering Model";
  };
  virtual OrientationType GetOrientationType(Hypothesis* currHypothesis) const;
};

class LexicalDirectionalReordering : public LexicalReordering {
 private:
  enum {Left = 0, Right = 1};
 public:
 LexicalDirectionalReordering(const std::string &filePath, 
							  const std::vector<float>& w, 
							  Direction direction, 
							  Condition condition, 
							  std::vector< FactorType >& f_factors, 
							  std::vector< FactorType >& e_factors)
   : LexicalReordering(filePath, w, direction, condition, f_factors, e_factors){
   std::cerr << "Created lexical directional Reordering\n";
  }
 public:
  virtual int GetNumOrientationTypes() const {
    return 2; 
  };
  virtual std::string GetScoreProducerDescription() const {
    return "Directional Lexical Reordering Model";
  };
  virtual OrientationType GetOrientationType(Hypothesis* currHypothesis) const;
};

//#endif //LEXICAL_REORDERING_H
