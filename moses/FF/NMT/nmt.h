#include <iostream>
#include <algorithm>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "mblas/base_matrix.h"

class Weights;
class Vocab;
class Encoder;
class Decoder;
class States;

class StateInfo;
typedef boost::shared_ptr<StateInfo> StateInfoPtr;

class NMT {
  public:
    NMT(const boost::shared_ptr<Weights> model,
        const boost::shared_ptr<Vocab> src,
        const boost::shared_ptr<Vocab> trg);
  
    static size_t GetDevices(size_t = 1);
    void SetDevice();
    size_t GetDevice();
  
    static boost::shared_ptr<Weights> NewModel(const std::string& path, size_t device = 0);
  
    static boost::shared_ptr<Vocab> NewVocab(const std::string& path);
  
    void CalcSourceContext(const std::vector<std::string>& s);
    
    StateInfoPtr EmptyState();
    
    void FilterTargetVocab(const std::vector<std::string>& filter);
    
    void MakeStep(
      const std::vector<std::string>& nextWords,
      const std::vector<std::string>& lastWords,
      std::vector<StateInfoPtr>& inputStates,
      std::vector<double>& logProbs,
      std::vector<StateInfoPtr>& nextStates,
      std::vector<bool>& unks);
  
    void ClearStates();
    
  private:
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
