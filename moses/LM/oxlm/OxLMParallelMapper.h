#pragma once

#include "moses/LM/oxlm/OxLMMapper.h"

namespace Moses {

class OxLMParallelMapper : public OxLMMapper {
 public:
  OxLMParallelMapper(const boost::shared_ptr<oxlm::Vocabulary>& vocab);

  int convertSource(const Moses::Factor* factor) const;

 private:
  Coll moses2SourceOxlm;
  int kSOURCE_UNKNOWN;
};

} // namespace Moses
