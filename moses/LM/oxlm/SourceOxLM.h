#pragma once

#include <vector>

#include "lbl/model.h"
#include "lbl/query_cache.h"

#include "moses/LM/BilingualLM.h"

namespace Moses {

class SourceOxLM : public BilingualLM {
 public:
	SourceOxLM(const std::string &line);

  ~SourceOxLM();

 private:
  virtual float Score(
      std::vector<int>& source_words,
      std::vector<int>& target_words) const;

  virtual int LookUpNeuralLMWord(const std::string& str) const;

  virtual void initSharedPointer() const;

  virtual void loadModel();

  virtual bool parseAdditionalSettings(
      const std::string& key,
      const std::string& value);

 protected:
  oxlm::SourceFactoredLM model;
};

} // namespace Moses
