#pragma once

#include <stack>
#include <vector>

#include "util/string_piece.hh"

#include "moses/FactorCollection.h"
#include "moses/TypeDef.h"

#include "HyperPath.h"
#include "TreeFragmentTokenizer.h"

namespace Moses
{
namespace Syntax
{
namespace F2S
{

// Parses a string representation of a tree fragment, adding the terminals
// and non-terminals to FactorCollection::Instance() and building a
// HyperPath object.
//
// This class is designed to be used during rule table loading.  Since every
// rule has a tree fragment on the source-side, Load() may be called millions
// of times.  The algorithm therefore sacrifices readability for speed and
// shoehorns everything into two passes over the input token sequence.
//
class HyperPathLoader
{
public:
  void Load(const StringPiece &, HyperPath &);

private:
  struct NodeTuple {
    int index;          // Preorder index of the node.
    int parent;         // Preorder index of the node's parent.
    int depth;          // Depth of the node.
    std::size_t symbol; // Either the factor ID of a tree terminal/non-terminal
    // or for virtual nodes, HyperPath::kEpsilon.
  };

  // Determine the height of the current tree fragment (stored in m_tokenSeq).
  int DetermineHeight() const;

  // Generate the preorder sequence of NodeTuples for the current tree fragment,
  // including virtual nodes.
  void GenerateNodeTupleSeq(int height);

  const Factor *AddTerminalFactor(const StringPiece &s) {
    return FactorCollection::Instance().AddFactor(s, false);
  }

  const Factor *AddNonTerminalFactor(const StringPiece &s) {
    return FactorCollection::Instance().AddFactor(s, true);
  }

  std::vector<TreeFragmentToken> m_tokenSeq;
  std::vector<NodeTuple> m_nodeTupleSeq;
  std::stack<int> m_parentStack;
};

}  // namespace F2S
}  // namespace Syntax
}  // namespace Moses
