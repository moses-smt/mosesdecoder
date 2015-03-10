#pragma once

#include <vector>
#include <string>

#include "Forest.h"

namespace MosesTraining
{
namespace Syntax
{
namespace PostprocessEgretForests
{

struct SplitPoint {
  int tokenPos;
  int charPos;
  std::string connector;
};

void MarkSplitPoints(const std::vector<SplitPoint> &, Forest &);

void MarkSplitPoints(const std::vector<SplitPoint> &, std::string &);

}  // namespace PostprocessEgretForests
}  // namespace Syntax
}  // namespace MosesTraining
