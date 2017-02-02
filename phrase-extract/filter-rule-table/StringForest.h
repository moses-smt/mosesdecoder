#pragma once

#include <string>

#include "Forest.h"

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

struct StringForestValue {
  std::string symbol;  // terminal or non-terminal (without square brackets)
  std::size_t start;
  std::size_t end;
};

typedef Forest<StringForestValue> StringForest;

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace Moses
