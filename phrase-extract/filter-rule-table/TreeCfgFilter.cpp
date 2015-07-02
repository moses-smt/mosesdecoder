#include "TreeCfgFilter.h"

#include <algorithm>

#include "util/string_piece_hash.hh"

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

TreeCfgFilter::TreeCfgFilter(
  const std::vector<boost::shared_ptr<SyntaxTree> > &sentences)
{
}

void TreeCfgFilter::Filter(std::istream &in, std::ostream &out)
{
  // TODO Implement filtering!
  std::string line;
  while (std::getline(in, line)) {
    out << line << std::endl;
  }
}

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining
