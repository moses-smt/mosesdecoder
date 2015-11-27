#include "HyperPathLoader.h"

#include "TreeFragmentTokenizer.h"

namespace Moses
{
namespace Syntax
{
namespace F2S
{

void HyperPathLoader::Load(const StringPiece &s, HyperPath &path)
{
  path.nodeSeqs.clear();
  // Tokenize the string and store the tokens in m_tokenSeq.
  m_tokenSeq.clear();
  for (TreeFragmentTokenizer p(s); p != TreeFragmentTokenizer(); ++p) {
    m_tokenSeq.push_back(*p);
  }
  // Determine the height of the tree fragment.
  int height = DetermineHeight();
  // Ensure path contains the correct number of elements.
  path.nodeSeqs.resize(height+1);
  // Generate the fragment's NodeTuple sequence and store it in m_nodeTupleSeq.
  GenerateNodeTupleSeq(height);
  // Fill the HyperPath.
  for (int depth = 0; depth <= height; ++depth) {
    int prevParent = -1;
// TODO Generate one node tuple sequence for each depth instead of one
// TODO sequence that contains node tuples at every depth
    for (std::vector<NodeTuple>::const_iterator p = m_nodeTupleSeq.begin();
         p != m_nodeTupleSeq.end(); ++p) {
      const NodeTuple &tuple = *p;
      if (tuple.depth != depth) {
        continue;
      }
      if (prevParent != -1 && tuple.parent != prevParent) {
        path.nodeSeqs[depth].push_back(HyperPath::kComma);
      }
      path.nodeSeqs[depth].push_back(tuple.symbol);
      prevParent = tuple.parent;
    }
  }
}

int HyperPathLoader::DetermineHeight() const
{
  int height = 0;
  int maxHeight = 0;
  std::size_t numTokens = m_tokenSeq.size();
  for (std::size_t i = 0; i < numTokens; ++i) {
    if (m_tokenSeq[i].type == TreeFragmentToken_LSB) {
      assert(i+2 < numTokens);
      // Does this bracket indicate the start of a subtree or the start of
      // a non-terminal leaf?
      if (m_tokenSeq[i+2].type != TreeFragmentToken_RSB) {  // It's a subtree.
        maxHeight = std::max(++height, maxHeight);
      } else {  // It's a non-terminal leaf: jump to its end.
        i += 2;
      }
    } else if (m_tokenSeq[i].type == TreeFragmentToken_RSB) {
      --height;
    }
  }
  return maxHeight;
}

void HyperPathLoader::GenerateNodeTupleSeq(int height)
{
  m_nodeTupleSeq.clear();

  // Initialize the stack of parent indices.
  assert(m_parentStack.empty());
  m_parentStack.push(-1);

  // Initialize a temporary tuple that tracks the state as we iterate over
  // the tree fragment tokens.
  NodeTuple tuple;
  tuple.index = -1;
  tuple.parent = -1;
  tuple.depth = -1;
  tuple.symbol = HyperPath::kEpsilon;

  // Iterate over the tree fragment tokens.
  std::size_t numTokens = m_tokenSeq.size();
  for (std::size_t i = 0; i < numTokens; ++i) {
    if (m_tokenSeq[i].type == TreeFragmentToken_LSB) {
      assert(i+2 < numTokens);
      // Does this bracket indicate the start of a subtree or the start of
      // a non-terminal leaf?
      if (m_tokenSeq[i+2].type != TreeFragmentToken_RSB) {  // It's a subtree.
        ++tuple.index;
        tuple.parent = m_parentStack.top();
        m_parentStack.push(tuple.index);
        ++tuple.depth;
        tuple.symbol = AddNonTerminalFactor(m_tokenSeq[++i].value)->GetId();
        m_nodeTupleSeq.push_back(tuple);
      } else {  // It's a non-terminal leaf.
        ++tuple.index;
        tuple.parent = m_parentStack.top();
        ++tuple.depth;
        tuple.symbol = AddNonTerminalFactor(m_tokenSeq[++i].value)->GetId();
        m_nodeTupleSeq.push_back(tuple);
        // Add virtual nodes if required.
        if (tuple.depth < height) {
          int origDepth = tuple.depth;
          m_parentStack.push(tuple.index);
          for (int depth = origDepth+1; depth <= height; ++depth) {
            ++tuple.index;
            tuple.parent = m_parentStack.top();
            m_parentStack.push(tuple.index);
            tuple.depth = depth;
            tuple.symbol = HyperPath::kEpsilon;
            m_nodeTupleSeq.push_back(tuple);
          }
          for (int depth = origDepth; depth <= height; ++depth) {
            m_parentStack.pop();
          }
          tuple.depth = origDepth;
        }
        --tuple.depth;
        // Skip over the closing bracket.
        ++i;
      }
    } else if (m_tokenSeq[i].type == TreeFragmentToken_WORD) {
      // Token i is a word that doesn't follow a bracket.  This must be a
      // terminal since all non-terminals are either non-leaves (which follow
      // an opening bracket) or are enclosed in brackets.
      ++tuple.index;
      tuple.parent = m_parentStack.top();
      ++tuple.depth;
      tuple.symbol = AddTerminalFactor(m_tokenSeq[i].value)->GetId();
      m_nodeTupleSeq.push_back(tuple);
      // Add virtual nodes if required.
      if (m_tokenSeq[i+1].type == TreeFragmentToken_RSB &&
          tuple.depth < height) {
        int origDepth = tuple.depth;
        m_parentStack.push(tuple.index);
        for (int depth = origDepth+1; depth <= height; ++depth) {
          ++tuple.index;
          tuple.parent = m_parentStack.top();
          m_parentStack.push(tuple.index);
          tuple.depth = depth;
          tuple.symbol = HyperPath::kEpsilon;
          m_nodeTupleSeq.push_back(tuple);
        }
        for (int depth = origDepth; depth <= height; ++depth) {
          m_parentStack.pop();
        }
        tuple.depth = origDepth;
      }
      --tuple.depth;
    } else if (m_tokenSeq[i].type == TreeFragmentToken_RSB) {
      m_parentStack.pop();
      --tuple.depth;
    }
  }

  // Remove the -1 parent index.
  m_parentStack.pop();
}

}  // namespace F2S
}  // namespace Syntax
}  // namespace Moses
