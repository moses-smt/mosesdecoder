#pragma once

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector> 

namespace Josiah {

class TranslationDelta;
  
class SampleAcceptor {
public:
  SampleAcceptor() {}
  virtual ~SampleAcceptor() {}
  virtual size_t choose(const std::vector<TranslationDelta*>& deltas) = 0;
  void getScores(const std::vector<TranslationDelta*>& deltas, std::vector<double>& scores);
  void normalize(std::vector<double>& scores); 
  double getRandom() ;
  size_t getSample(const std::vector<double>& scores, double random);
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
  size_t maxScore(const std::vector<TranslationDelta*>& deltas);
};
  
class TargetAssigner {
public:
  TargetAssigner(const std::string& name) : m_name(name) {}
  virtual ~TargetAssigner(){}
  size_t virtual getTarget(const std::vector<TranslationDelta*>& deltas, const TranslationDelta* noChangeDelta) = 0;
  std::string m_name;
};

class BestNeighbourTgtAssigner : public TargetAssigner {
public:
  BestNeighbourTgtAssigner() : TargetAssigner("Best") {}
  virtual ~BestNeighbourTgtAssigner(){}
  size_t virtual getTarget(const std::vector<TranslationDelta*>& deltas, const TranslationDelta* noChangeDelta);
};

class ClosestBestNeighbourTgtAssigner : public TargetAssigner {
public:
  ClosestBestNeighbourTgtAssigner(): TargetAssigner("CBN") {}
  virtual ~ClosestBestNeighbourTgtAssigner(){}
  size_t virtual getTarget(const std::vector<TranslationDelta*>& deltas, const TranslationDelta* noChangeDelta);
};

//class ChiangBestNeighbourTgtAssigner : public TargetAssigner {
//  public:
//    ChiangBestNeighbourTgtAssigner(){}
//    virtual ~ChiangBestNeighbourTgtAssigner(){}
//    size_t virtual getTarget(const std::vector<TranslationDelta*>& deltas, const TranslationDelta* noChangeDelta);
//};
  
}
