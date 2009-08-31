#pragma once

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector> 
#include "ScoreProducer.h"
#include "LanguageModel.h"

namespace Josiah {

class TranslationDelta;
  
class SampleAcceptor {
public:
  SampleAcceptor() {}
  virtual ~SampleAcceptor() {}
  virtual TranslationDelta* choose(const std::vector<TranslationDelta*>& deltas) = 0;
  void getScores(const std::vector<TranslationDelta*>& deltas, std::vector<double>& scores);
  void normalize(std::vector<double>& scores); 
  double getRandom() ;
  size_t getSample(const std::vector<double>& scores, double random);
};

class FixedTempAcceptor : public SampleAcceptor {
public:
  FixedTempAcceptor(float temp) {m_temp = temp;}
  virtual ~FixedTempAcceptor() {}
  virtual TranslationDelta*choose(const std::vector<TranslationDelta*>& deltas) ;
  void SetTemp(float temp) { m_temp = temp;}
private:
  float m_temp;
};

class RegularAcceptor : public SampleAcceptor {
public:
  RegularAcceptor() {}
  virtual ~RegularAcceptor() {}
  virtual TranslationDelta* choose(const std::vector<TranslationDelta*>& deltas) ;
};

class GreedyAcceptor : public SampleAcceptor {
public:
  GreedyAcceptor() {}
  virtual ~GreedyAcceptor() {}
  virtual TranslationDelta* choose(const std::vector<TranslationDelta*>& deltas) ;
  size_t maxScore(const std::vector<TranslationDelta*>& deltas);
};
  
class MHAcceptor {
  public:
    MHAcceptor() {}
    ~MHAcceptor() {}
    TranslationDelta* choose(TranslationDelta* curr, TranslationDelta* next);
    void addScoreProducer(Moses::ScoreProducer* sp) {m_scoreProducers.push_back(sp);} 
  void setProposalLMInfo(const std::map <Moses::LanguageModel*, int> & proposal) { m_proposalLMInfo = proposal;}
  void setTargetLMInfo(const std::map <Moses::LanguageModel*, int> & target) { m_targetLMInfo = target;}
  static long mhtotal;
  static long acceptanceCtr;
  private:  
    std::vector <Moses::ScoreProducer*> m_scoreProducers;
  std::map <Moses::LanguageModel*, int> m_targetLMInfo, m_proposalLMInfo;
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
