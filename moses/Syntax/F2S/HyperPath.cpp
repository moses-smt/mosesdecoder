#include "HyperPath.h"

#include <limits>

namespace Moses
{
namespace Syntax
{
namespace F2S
{

const std::size_t HyperPath::kEpsilon =
  std::numeric_limits<std::size_t>::max()-1;

const std::size_t HyperPath::kComma =
  std::numeric_limits<std::size_t>::max()-2;

}  // namespace F2S
}  // namespace Syntax
}  // namespace Moses
