#pragma once

#include <istream>
#include <ostream>
#include <string>
#include <vector>

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

class StringBasedFilter
{
public:
  StringBasedFilter(const std::vector<std::vector<std::string> > &);

  void Filter(std::istream &, std::ostream &);
};

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining
