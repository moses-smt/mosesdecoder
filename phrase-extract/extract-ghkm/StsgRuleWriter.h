#pragma once
#ifndef EXTRACT_GHKM_STSG_RULE_WRITER_H_
#define EXTRACT_GHKM_STSG_RULE_WRITER_H_

#include <ostream>

#include "Subgraph.h"

namespace MosesTraining
{
namespace Syntax
{
namespace GHKM
{

struct Options;
class StsgRule;
class Symbol;

class StsgRuleWriter
{
public:
  StsgRuleWriter(std::ostream &fwd, std::ostream &inv, const Options &options)
    : m_fwd(fwd)
    , m_inv(inv)
    , m_options(options) {}

  void Write(const StsgRule &rule);

private:
  // Disallow copying
  StsgRuleWriter(const StsgRuleWriter &);
  StsgRuleWriter &operator=(const StsgRuleWriter &);

  std::ostream &m_fwd;
  std::ostream &m_inv;
  const Options &m_options;
};

}  // namespace GHKM
}  // namespace Syntax
}  // namespace MosesTraining

#endif
