#pragma once

#include <vector>

#include "moses/Factor.h"

namespace Moses
{
namespace Syntax
{
namespace F2S
{

// A HyperPath for representing the source-side tree fragment of a
// tree-to-string rule.  See this paper:
//
//  Hui Zhang, Min Zhang, Haizhou Li, and Chew Lim Tan
//  "Fast Translation Rule Matching for Syntax-based Statistical Machine
//   Translation"
//  In proceedings of EMNLP 2009
//
struct HyperPath {
public:
  typedef std::vector<std::size_t> NodeSeq;

  static const std::size_t kEpsilon;
  static const std::size_t kComma;

  std::vector<NodeSeq> nodeSeqs;
};

}  // namespace F2S
}  // namespace Syntax
}  // namespace Moses
