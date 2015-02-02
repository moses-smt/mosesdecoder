#include "StringBasedFilter.h"

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

StringBasedFilter::StringBasedFilter(
  const std::vector<std::vector<std::string> > &sentences)
{
}

void StringBasedFilter::Filter(std::istream &in, std::ostream &out)
{
  std::string line;
  int lineNum = 0;
  while (std::getline(in, line)) {
    ++lineNum;
    out << line << std::endl;
  }
}

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining
