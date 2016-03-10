#include <iostream>
#include <algorithm>
#include <vector>
#include <set>
#include <boost/shared_ptr.hpp>

#include "mblas/base_matrix.h"

class Weights;
class Vocab;
class Encoder;
class Decoder;
class States;

class StateInfo;
typedef boost::shared_ptr<StateInfo> StateInfoPtr;

typedef std::vector<size_t> Batch;
typedef std::vector<Batch> Batches;
typedef std::vector<StateInfoPtr> StateInfos;
typedef std::vector<float> Scores;
typedef std::vector<size_t> LastWords;


class NMT {
  public:
    NMT(const boost::shared_ptr<Weights> model,
        const boost::shared_ptr<Vocab> src,
        const boost::shared_ptr<Vocab> trg);
  
    const boost::shared_ptr<Weights> GetModel() {
      return w_;
    }
  
    static size_t GetDevices(size_t = 1);
    void SetDevice();
    size_t GetDevice();
  
    void SetDebug(bool debug) {
      debug_ = debug;
    }
  
    static boost::shared_ptr<Weights> NewModel(const std::string& path, size_t device = 0);
  
    static boost::shared_ptr<Vocab> NewVocab(const std::string& path);
  
    void CalcSourceContext(const std::vector<std::string>& s);
    
    StateInfoPtr EmptyState();
    
    void PrintState(StateInfoPtr);
    
    void FilterTargetVocab(const std::set<std::string>& filter);
    
    size_t TargetVocab(const std::string& str);
    
    void BatchSteps(const Batches& batches, LastWords& lastWords,
                    Scores& probs, Scores& unks, StateInfos& stateInfos,
                    bool firstWord);
  
    void OnePhrase(
      const std::vector<std::string>& phrase,
      const std::string& lastWord,
      bool firstWord,
      StateInfoPtr inputState,
      float& prob, size_t& unks,
      StateInfoPtr& outputState);

    void MakeStep(
      const std::vector<std::string>& nextWords,
      const std::vector<std::string>& lastWords,
      std::vector<StateInfoPtr>& inputStates,
      std::vector<double>& logProbs,
      std::vector<StateInfoPtr>& nextStates,
      std::vector<bool>& unks);
  
    void ClearStates();
    
  private:
    bool debug_;
    
    const boost::shared_ptr<Weights> w_;
    const boost::shared_ptr<Vocab> src_;
    const boost::shared_ptr<Vocab> trg_;
    
    boost::shared_ptr<Encoder> encoder_;
    boost::shared_ptr<Decoder> decoder_;
    
    boost::shared_ptr<mblas::BaseMatrix> SourceContext_;
    
    boost::shared_ptr<States> states_;
    bool firstWord_;
    
    std::vector<size_t> filteredId_;
};
