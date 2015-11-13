#pragma once

#include "Forest.h"

namespace Moses
{
namespace Syntax
{
namespace F2S
{

// Base class for rule matchers.
template<typename Callback>
class RuleMatcher
{
public:
  virtual ~RuleMatcher() {}

  virtual void EnumerateHyperedges(const Forest::Vertex &, Callback &) = 0;
};

}  // F2S
}  // Syntax
}  // Moses
