#include "BilingualLM.h"

namespace nplm {
  class neuralLM;
}

namespace Moses {

  class BilingualLM_NPLM : public BilingualLM 
  {
  public:
    BilingualLM_NPLM(const std::string &line);
    
  private:
    mutable nplm::neuralLM *m_neuralLM_shared;
    mutable boost::thread_specific_ptr<nplm::neuralLM> m_neuralLM;
    
    bool premultiply;
    int neuralLM_cache;
    
    float Score(std::vector<int>& source_words, std::vector<int>& target_words) const;
    int LookUpNeuralLMWord(const std::string str) const;
    void initSharedPointer() const;
    void loadModel() const;
    bool parseAdditionalSettings(const std::string& key, const std::string& value) ; //Gives pure virtual error...

  };
}