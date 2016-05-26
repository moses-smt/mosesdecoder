#include "InputTreeBuilder.h"

#include "moses/StaticData.h"

namespace Moses
{
namespace Syntax
{
namespace T2S
{

InputTreeBuilder::InputTreeBuilder(std::vector<FactorType> const& oFactors)
  : m_outputFactorOrder(oFactors)
{
}

void InputTreeBuilder::Build(const TreeInput &in,
                             const std::string &topLevelLabel,
                             InputTree &out)
{
  CreateNodes(in, topLevelLabel, out);
  ConnectNodes(out);
}

// Create the InputTree::Node objects but do not connect them.
void InputTreeBuilder::CreateNodes(const TreeInput &in,
                                   const std::string &topLevelLabel,
                                   InputTree &out)
{
  // Get the input sentence word count.  This includes the <s> and </s> symbols.
  const std::size_t numWords = in.GetSize();

  // Get the parse tree non-terminal nodes.  The parse tree covers the original
  // sentence only, not the <s> and </s> symbols, so at this point there is
  // no top-level node.
  std::vector<XMLParseOutput> xmlNodes = in.GetLabelledSpans();

  // Sort the XML nodes into post-order.  Prior to sorting they will be in the
  // order that TreeInput created them.  Usually that will be post-order, but
  // if, for example, the tree was binarized by relax-parse then it won't be.
  // In all cases, we assume that if two nodes cover the same span then the
  // first one is the lowest.
  SortXmlNodesIntoPostOrder(xmlNodes);

  // Copy the parse tree non-terminal nodes, but offset the ranges by 1 (to
  // allow for the <s> symbol at position 0).
  std::vector<XMLParseOutput> nonTerms;
  nonTerms.reserve(xmlNodes.size()+1);
  for (std::vector<XMLParseOutput>::const_iterator p = xmlNodes.begin();
       p != xmlNodes.end(); ++p) {
    std::size_t start = p->m_range.GetStartPos();
    std::size_t end = p->m_range.GetEndPos();
    nonTerms.push_back(XMLParseOutput(p->m_label, Range(start+1, end+1)));
  }
  // Add a top-level node that also covers <s> and </s>.
  nonTerms.push_back(XMLParseOutput(topLevelLabel, Range(0, numWords-1)));

  // Allocate space for the InputTree nodes.  In the case of out.nodes, this
  // step is essential because once created the PVertex objects must not be
  // moved around (through vector resizing) because InputTree keeps pointers
  // to them.
  out.nodes.reserve(numWords + nonTerms.size());
  out.nodesAtPos.resize(numWords);

  // Create the InputTree::Node objects.
  int prevStart = -1;
  int prevEnd = -1;
  for (std::vector<XMLParseOutput>::const_iterator p = nonTerms.begin();
       p != nonTerms.end(); ++p) {
    int start = static_cast<int>(p->m_range.GetStartPos());
    int end = static_cast<int>(p->m_range.GetEndPos());

    // Check if we've started ascending a new subtree.
    if (start != prevStart && end != prevEnd) {
      // Add a node for each terminal to the left of or below the first
      // nonTerm child of the subtree.
      for (int i = prevEnd+1; i <= end; ++i) {
        PVertex v(Range(i, i), in.GetWord(i));
        out.nodes.push_back(InputTree::Node(v));
        out.nodesAtPos[i].push_back(&out.nodes.back());
      }
    }
    // Add a node for the non-terminal.
    Word w(true);
    w.CreateFromString(Moses::Output, m_outputFactorOrder, p->m_label, true);
    PVertex v(Range(start, end), w);
    out.nodes.push_back(InputTree::Node(v));
    out.nodesAtPos[start].push_back(&out.nodes.back());

    prevStart = start;
    prevEnd = end;
  }
}

// Connect the nodes by filling in the node.children vectors.
void InputTreeBuilder::ConnectNodes(InputTree &out)
{
  // Create a vector that records the parent of each node (except the root).
  std::vector<InputTree::Node*> parents(out.nodes.size(), NULL);
  for (std::size_t i = 0; i < out.nodes.size()-1; ++i) {
    const InputTree::Node &node = out.nodes[i];
    std::size_t start = node.pvertex.span.GetStartPos();
    std::size_t end = node.pvertex.span.GetEndPos();
    // Find the next node (in post-order) that completely covers node's span.
    std::size_t j = i+1;
    while (true) {
      const InputTree::Node &succ = out.nodes[j];
      std::size_t succStart = succ.pvertex.span.GetStartPos();
      std::size_t succEnd = succ.pvertex.span.GetEndPos();
      if (succStart <= start && succEnd >= end) {
        break;
      }
      ++j;
    }
    parents[i] = &(out.nodes[j]);
  }

  // Add each node to its parent's list of children (except the root).
  for (std::size_t i = 0; i < out.nodes.size()-1; ++i) {
    InputTree::Node &child = out.nodes[i];
    InputTree::Node &parent = *(parents[i]);
    parent.children.push_back(&child);
  }
}

void InputTreeBuilder::SortXmlNodesIntoPostOrder(
  std::vector<XMLParseOutput> &nodes)
{
  // Sorting is based on both the value of a node and its original position,
  // so for each node construct a pair containing both pieces of information.
  std::vector<std::pair<XMLParseOutput *, int> > pairs;
  pairs.reserve(nodes.size());
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    pairs.push_back(std::make_pair(&(nodes[i]), i));
  }

  // Sort the pairs.
  std::sort(pairs.begin(), pairs.end(), PostOrderComp);

  // Replace the original node sequence with the correctly sorted sequence.
  std::vector<XMLParseOutput> tmp;
  tmp.reserve(nodes.size());
  for (std::size_t i = 0; i < pairs.size(); ++i) {
    tmp.push_back(nodes[pairs[i].second]);
  }
  nodes.swap(tmp);
}

// Comparison function used by SortXmlNodesIntoPostOrder.
bool InputTreeBuilder::PostOrderComp(const std::pair<XMLParseOutput *, int> &x,
                                     const std::pair<XMLParseOutput *, int> &y)
{
  std::size_t xStart = x.first->m_range.GetStartPos();
  std::size_t xEnd = x.first->m_range.GetEndPos();
  std::size_t yStart = y.first->m_range.GetStartPos();
  std::size_t yEnd = y.first->m_range.GetEndPos();

  if (xEnd == yEnd) {
    if (xStart == yStart) {
      return x.second < y.second;
    } else {
      return xStart > yStart;
    }
  } else {
    return xEnd < yEnd;
  }
}

}  // T2S
}  // Syntax
}  // Moses
