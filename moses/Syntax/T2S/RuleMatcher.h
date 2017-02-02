#pragma once

#include "InputTree.h"

namespace Moses
{
namespace Syntax
{
namespace T2S
{

// Base class for rule matchers.
template<typename Callback>
class RuleMatcher
{
public:
  virtual ~RuleMatcher() {}

  virtual void EnumerateHyperedges(const InputTree::Node &, Callback &) = 0;
};

}  // T2S
}  // Syntax
}  // Moses
