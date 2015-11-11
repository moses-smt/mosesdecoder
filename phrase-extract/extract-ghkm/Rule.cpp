#include "Rule.h"

#include "Node.h"
#include "Subgraph.h"

namespace MosesTraining
{
namespace Syntax
{
namespace GHKM
{

int Rule::Scope(const std::vector<Symbol> &symbols)
{
  int scope = 0;
  bool predIsNonTerm = false;
  if (symbols[0].GetType() == NonTerminal) {
    ++scope;
    predIsNonTerm = true;
  }
  for (std::size_t i = 1; i < symbols.size(); ++i) {
    bool isNonTerm = symbols[i].GetType() == NonTerminal;
    if (isNonTerm && predIsNonTerm) {
      ++scope;
    }
    predIsNonTerm = isNonTerm;
  }
  if (predIsNonTerm) {
    ++scope;
  }
  return scope;
}

bool Rule::PartitionOrderComp(const Node *a, const Node *b)
{
  const Span &aSpan = a->GetSpan();
  const Span &bSpan = b->GetSpan();
  assert(!aSpan.empty() && !bSpan.empty());
  return *(aSpan.begin()) < *(bSpan.begin());
}

}  // namespace GHKM
}  // namespace Syntax
}  // namespace MosesTraining
