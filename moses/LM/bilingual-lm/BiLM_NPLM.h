#include "moses/LM/BilingualLM.h"

namespace nplm {
  class neuralLM;
}

namespace Moses {

class BilingualLM_NPLM : public BilingualLM {
 public:
  BilingualLM_NPLM(const std::string &line);

 private:
  float Score(std::vector<int>& source_words, std::vector<int>& target_words) const;

  int getNeuralLMId(const Word& word, bool is_source_word) const;

  int LookUpNeuralLMWord(const std::string& str) const;

  void initSharedPointer() const;

  void loadModel();

  void SetParameter(const std::string& key, const std::string& value);

  const Word& getNullWord() const;

  nplm::neuralLM *m_neuralLM_shared;
  mutable boost::thread_specific_ptr<nplm::neuralLM> m_neuralLM;

  mutable std::map<const Factor*, int> neuralLMids;
  mutable boost::shared_mutex neuralLMids_lock;

  //const Factor* NULL_factor_overwrite;
  std::string NULL_string;
  bool NULL_overwrite;
  Word NULL_word;

  bool premultiply;
  bool factored;
  int neuralLM_cache;
  int unknown_word_id;
};

} // namespace Moses
