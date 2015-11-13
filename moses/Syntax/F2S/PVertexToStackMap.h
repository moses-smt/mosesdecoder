#pragma once

#include <boost/unordered_map.hpp>

#include "moses/Syntax/PVertex.h"
#include "moses/Syntax/SVertexStack.h"


namespace Moses
{
namespace Syntax
{
namespace F2S
{

typedef boost::unordered_map<const PVertex *, SVertexStack> PVertexToStackMap;

}  // namespace F2S
}  // namespace Syntax
}  // namespace Moses
