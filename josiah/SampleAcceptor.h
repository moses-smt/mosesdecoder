#pragma once

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector> 
#include "TranslationDelta.h" 
#include "GibbsOperator.h"

namespace Josiah {
  
class SampleAcceptor {
public:
  SampleAcceptor() {}
  virtual ~SampleAcceptor() {}
  virtual size_t choose(const std::vector<TranslationDelta*>& deltas) = 0;
  void getScores(const std::vector<TranslationDelta*>& deltas, vector<double>& scores);
  void normalize(vector<double>& scores); 
  double getRandom() ;
  size_t getSample(const vector<double>& scores, double random);
};

class FixedTempAcceptor : public SampleAcceptor {
public:
  FixedTempAcceptor(float temp) {m_temp = temp;}
  virtual ~FixedTempAcceptor() {}
  virtual size_t choose(const std::vector<TranslationDelta*>& deltas) ;
  void SetTemp(float temp) { m_temp = temp;}
private:
  float m_temp;
};

class RegularAcceptor : public SampleAcceptor {
public:
  RegularAcceptor() {}
  virtual ~RegularAcceptor() {}
  virtual size_t choose(const std::vector<TranslationDelta*>& deltas) ;
};

class GreedyAcceptor : public SampleAcceptor {
public:
  GreedyAcceptor() {}
  virtual ~GreedyAcceptor() {}
  virtual size_t choose(const std::vector<TranslationDelta*>& deltas) ;
  size_t maxScore(const vector<TranslationDelta*>& deltas);
};
  
class TargetAssigner {
public:
  TargetAssigner(){}
  virtual ~TargetAssigner(){}
  size_t virtual getTarget(const vector<TranslationDelta*>& deltas, const TranslationDelta* noChangeDelta) = 0;
};

class BestNeighbourTgtAssigner : public TargetAssigner {
public:
  BestNeighbourTgtAssigner(){}
  virtual ~BestNeighbourTgtAssigner(){}
  size_t virtual getTarget(const vector<TranslationDelta*>& deltas, const TranslationDelta* noChangeDelta);
};

class ClosestBestNeighbourTgtAssigner : public TargetAssigner {
public:
  ClosestBestNeighbourTgtAssigner(){}
  virtual ~ClosestBestNeighbourTgtAssigner(){}
  size_t virtual getTarget(const vector<TranslationDelta*>& deltas, const TranslationDelta* noChangeDelta);
};
  
}
