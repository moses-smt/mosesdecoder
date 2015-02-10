#pragma once

#include "moses/Syntax/F2S/Forest.h"

#include "InputTree.h"

namespace Moses
{
namespace Syntax
{
namespace T2S
{

// Constructs a F2S::Forest given a T2S::InputTree.
const F2S::Forest::Vertex *InputTreeToForest(const InputTree &, F2S::Forest &);

}  // T2S
}  // Syntax
}  // Moses
