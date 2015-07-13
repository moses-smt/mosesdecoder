#pragma once
#ifndef EXTRACT_GHKM_RULE_H_
#define EXTRACT_GHKM_RULE_H_

#include <string>
#include <vector>

#include "Alignment.h"

namespace MosesTraining
{
namespace Syntax
{
namespace GHKM
{

class Node;

enum SymbolType { Terminal, NonTerminal };

class Symbol
{
public:
  Symbol(const std::string &v, SymbolType t) : m_value(v) , m_type(t) {}

  const std::string &GetValue() const {
    return m_value;
  }
  SymbolType GetType() const {
    return m_type;
  }

private:
  std::string m_value;
  SymbolType m_type;
};

// Base class for ScfgRule and StsgRule.
class Rule
{
public:
  virtual ~Rule() {}

  const Alignment &GetAlignment() const {
    return m_alignment;
  }

  virtual int Scope() const = 0;

protected:
  static bool PartitionOrderComp(const Node *, const Node *);

  static int Scope(const std::vector<Symbol>&);

  Alignment m_alignment;
};

}  // namespace GHKM
}  // namespace Syntax
}  // namespace MosesTraining

#endif
