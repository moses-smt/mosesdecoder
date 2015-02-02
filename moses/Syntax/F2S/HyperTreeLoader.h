#pragma once

#include <istream>
#include <vector>

#include "moses/TypeDef.h"
#include "moses/Syntax/RuleTableFF.h"

#include "HyperTree.h"
#include "HyperTreeCreator.h"

namespace Moses
{
namespace Syntax
{
namespace F2S
{

class HyperTreeLoader : public HyperTreeCreator
{
 public:
  bool Load(const std::vector<FactorType> &input,
            const std::vector<FactorType> &output,
            const std::string &inFile,
            const RuleTableFF &,
            HyperTree &);
};

}  // namespace F2S
}  // namespace Syntax
}  // namespace Moses
