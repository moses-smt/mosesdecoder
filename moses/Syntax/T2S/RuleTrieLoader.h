#pragma once

#include <istream>
#include <vector>

#include "moses/TypeDef.h"
#include "moses/Syntax/RuleTableFF.h"

#include "RuleTrie.h"
#include "RuleTrieCreator.h"

namespace Moses
{
class AllOptions;
namespace Syntax
{
namespace T2S
{

class RuleTrieLoader : public RuleTrieCreator
{
public:
  bool Load(Moses::AllOptions const& opts,
            const std::vector<FactorType> &input,
            const std::vector<FactorType> &output,
            const std::string &inFile,
            const RuleTableFF &,
            RuleTrie &);
};

}  // namespace T2S
}  // namespace Syntax
}  // namespace Moses
