#pragma once

#include <string>

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

struct Options {
public:
  Options() {}

  // Positional options
  std::string model;
  std::string testSetFile;
};

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace Moses
