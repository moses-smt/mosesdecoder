#pragma once

#include "moses/LM/oxlm/OxLMMapper.h"

namespace Moses
{

class OxLMParallelMapper : public OxLMMapper
{
public:
  OxLMParallelMapper(
    const boost::shared_ptr<oxlm::Vocabulary>& vocab,
    bool pos_back_off,
    const FactorType& pos_factor_type);

  int convertSource(const Word& word) const;

private:
  Coll moses2SourceOxlm;
  int kSOURCE_UNKNOWN;
};

} // namespace Moses
