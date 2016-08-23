#pragma once

#include <istream>
#include <vector>

#include <boost/unordered_set.hpp>

#include "moses/TypeDef.h"
#include "moses/Syntax/RuleTableFF.h"

#include "HyperPath.h"
#include "HyperTree.h"
#include "HyperTreeCreator.h"

namespace Moses
{
class AllOptions;
namespace Syntax
{
namespace F2S
{

class HyperTreeLoader : public HyperTreeCreator
{
public:
  bool Load(AllOptions const& opts,
            const std::vector<FactorType> &input,
            const std::vector<FactorType> &output,
            const std::string &inFile,
            const RuleTableFF &,
            HyperTree &,
            boost::unordered_set<std::size_t> &);

private:
  void ExtractSourceTerminalSetFromHyperPath(
    const HyperPath &, boost::unordered_set<std::size_t> &);
};

}  // namespace F2S
}  // namespace Syntax
}  // namespace Moses
