#pragma once
#ifndef EXTRACT_GHKM_STSG_RULE_H_
#define EXTRACT_GHKM_STSG_RULE_H_

#include <vector>

#include "Rule.h"
#include "Subgraph.h"

namespace MosesTraining
{
namespace Syntax
{
namespace GHKM
{

class Node;

class StsgRule : public Rule
{
public:
  StsgRule(const Subgraph &fragment);

  const std::vector<Symbol> &GetSourceSide() const {
    return m_sourceSide;
  }
  const Subgraph &GetTargetSide() const {
    return m_targetSide;
  }
  const std::vector<int> &GetNonTermAlignment() const {
    return m_nonTermAlignment;
  }
  int Scope() const {
    return Rule::Scope(m_sourceSide);
  }

private:
  std::vector<Symbol> m_sourceSide;
  Subgraph m_targetSide;
  std::vector<int> m_nonTermAlignment;
};

}  // namespace GHKM
}  // namespace Syntax
}  // namespace MosesTraining

#endif
