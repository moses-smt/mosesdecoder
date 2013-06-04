//
// Oliver Wilson <oliver.wilson@ed.ac.uk>
//

#ifndef moses_LanguageModelLDHT_h
#define moses_LanguageModelLDHT_h

#include "moses/TypeDef.h"

namespace Moses
{

class ScoreIndexManager;
class LanguageModel;

LanguageModel* ConstructLDHTLM(const std::string& file,
                               ScoreIndexManager& manager,
                               FactorType factorType);
}  // namespace Moses.

#endif  // moses_LanguageModelLDHT_h

