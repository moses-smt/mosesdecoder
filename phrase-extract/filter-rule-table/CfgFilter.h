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

// Base class for StringCfgFilter and TreeCfgFilter, both of which filter rule
// tables where the source-side is CFG.
class CfgFilter
{
public:
  virtual ~CfgFilter() {}

  // Read a rule table from 'in' and filter it according to the test sentences.
  virtual void Filter(std::istream &in, std::ostream &out) = 0;

protected:
};

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining
