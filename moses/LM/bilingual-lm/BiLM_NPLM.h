#include "moses/LM/BilingualLM.h"
#include <boost/unordered_map.hpp>
#include <utility> //make_pair
#include <fstream> //Read vocabulary files

namespace nplm
{
class neuralLM;
}

namespace Moses
{

class BilingualLM_NPLM : public BilingualLM
{
public:
  BilingualLM_NPLM(const std::string &line);

private:
  float Score(std::vector<int>& source_words, std::vector<int>& target_words) const;

  int getNeuralLMId(const Word& word, bool is_source_word) const;

  void initSharedPointer() const;

  void loadModel();

  void SetParameter(const std::string& key, const std::string& value);

  const Word& getNullWord() const;

  nplm::neuralLM *m_neuralLM_shared;
  mutable boost::thread_specific_ptr<nplm::neuralLM> m_neuralLM;

  mutable boost::unordered_map<const Factor*, int> target_neuralLMids;
  mutable boost::unordered_map<const Factor*, int> source_neuralLMids;

  //const Factor* NULL_factor_overwrite;
  std::string NULL_string;
  bool NULL_overwrite;
  Word NULL_word;

  std::string source_vocab_path;
  std::string target_vocab_path;
  bool premultiply;
  bool factored;
  int neuralLM_cache;
  int source_unknown_word_id;
  int target_unknown_word_id;
};

} // namespace Moses
