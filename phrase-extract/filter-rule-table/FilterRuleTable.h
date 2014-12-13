#pragma once

#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>

#include "syntax-common/string_tree.h"

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

struct Options;

class FilterRuleTable
{
public:
  FilterRuleTable() : m_name("filter-rule-table") {}

  const std::string &GetName() const {
    return m_name;
  }

  int Main(int argc, char *argv[]);

private:
  void Error(const std::string &) const;

  // Filter rule table (on std::cin) for test set (string version).
  void Filter(const std::vector<std::vector<std::string> > &);

  // Filter rule table (on std::cin) for test set (parse tree version).
  void Filter(const std::vector<boost::shared_ptr<StringTree> > &);

  void ProcessOptions(int, char *[], Options &) const;

  void ReadTokens(const std::string &, std::vector<std::string> &);

  std::string m_name;
};

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining
