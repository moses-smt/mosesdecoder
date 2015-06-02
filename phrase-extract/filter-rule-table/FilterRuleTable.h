#pragma once

#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>

#include "SyntaxTree.h"

#include "StringForest.h"

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
  void Filter(const std::vector<boost::shared_ptr<SyntaxTree> > &);

  void ProcessOptions(int, char *[], Options &) const;

  // Read test set (string version)
  void ReadTestSet(std::istream &,
                   std::vector<boost::shared_ptr<std::string> > &);

  // Read test set (tree version)
  void ReadTestSet(std::istream &,
                   std::vector<boost::shared_ptr<SyntaxTree> > &);

  // Read test set (forest version)
  void ReadTestSet(std::istream &,
                   std::vector<boost::shared_ptr<StringForest> > &);

  void Warn(const std::string &) const;

  std::string m_name;
};

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining
