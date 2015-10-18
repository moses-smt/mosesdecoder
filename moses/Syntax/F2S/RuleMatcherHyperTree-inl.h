#pragma once

namespace Moses
{
namespace Syntax
{
namespace F2S
{

template<typename Callback>
RuleMatcherHyperTree<Callback>::RuleMatcherHyperTree(const HyperTree &ruleTrie)
  : m_ruleTrie(ruleTrie)
{
}

template<typename Callback>
void RuleMatcherHyperTree<Callback>::EnumerateHyperedges(
  const Forest::Vertex &v, Callback &callback)
{
  const HyperTree::Node &root = m_ruleTrie.GetRootNode();
  HyperPath::NodeSeq nodeSeq(1, v.pvertex.symbol[0]->GetId());
  const HyperTree::Node *child = root.GetChild(nodeSeq);
  if (!child) {
    return;
  }

  m_hyperedge.head = const_cast<PVertex*>(&v.pvertex);

  // Initialize the queue.
  MatchItem item;
  item.annotatedFNS.fns = FNS(1, &v);
  item.trieNode = child;
  m_queue.push(item);

  while (!m_queue.empty()) {
    MatchItem item = m_queue.front();
    m_queue.pop();
    if (item.trieNode->HasRules()) {
      const FNS &fns = item.annotatedFNS.fns;
      // Set the output hyperedge's tail.
      m_hyperedge.tail.clear();
      for (FNS::const_iterator p = fns.begin(); p != fns.end(); ++p) {
        const Forest::Vertex *v = *p;
        m_hyperedge.tail.push_back(const_cast<PVertex *>(&(v->pvertex)));
      }
      // Set the output hyperedge label's input weight.
      m_hyperedge.label.inputWeight = 0.0f;
      for (std::vector<const Forest::Hyperedge *>::const_iterator
           p = item.annotatedFNS.fragment.begin();
           p != item.annotatedFNS.fragment.end(); ++p) {
        m_hyperedge.label.inputWeight += (*p)->weight;
      }
      // Set the output hyperedge label's translation set pointer.
      m_hyperedge.label.translations
      = item.trieNode->GetTargetPhraseCollection();
      // Pass the output hyperedge to the callback.
      callback(m_hyperedge);
    }
    PropagateNextLexel(item);
  }
}

template<typename Callback>
void RuleMatcherHyperTree<Callback>::PropagateNextLexel(const MatchItem &item)
{
  std::vector<AnnotatedFNS> tfns;
  std::vector<AnnotatedFNS> rfns;
  std::vector<AnnotatedFNS> rfns2;

  const HyperTree::Node &trieNode = *(item.trieNode);
  const HyperTree::Node::Map &map = trieNode.GetMap();

  for (HyperTree::Node::Map::const_iterator p = map.begin();
       p != map.end(); ++p) {
    const HyperPath::NodeSeq &edgeLabel = p->first;
    const HyperTree::Node &child = p->second;

    const int numSubSeqs = CountCommas(edgeLabel) + 1;

    std::size_t pos = 0;
    for (int i = 0; i < numSubSeqs; ++i) {
      const FNS &fns = item.annotatedFNS.fns;
      tfns.clear();
      if (edgeLabel[pos] == HyperPath::kEpsilon) {
        AnnotatedFNS x;
        x.fns = FNS(1, fns[i]);
        tfns.push_back(x);
        pos += 2;
      } else {
        const int subSeqLength = SubSeqLength(edgeLabel, pos);
        const std::vector<Forest::Hyperedge*> &incoming = fns[i]->incoming;
        for (std::vector<Forest::Hyperedge *>::const_iterator q =
               incoming.begin(); q != incoming.end(); ++q) {
          const Forest::Hyperedge &edge = **q;
          if (MatchChildren(edge.tail, edgeLabel, pos, subSeqLength)) {
            tfns.resize(tfns.size()+1);
            tfns.back().fns.assign(edge.tail.begin(), edge.tail.end());
            tfns.back().fragment.push_back(&edge);
          }
        }
        pos += subSeqLength + 1;
      }
      if (tfns.empty()) {
        rfns.clear();
        break;
      } else if (i == 0) {
        rfns.swap(tfns);
      } else {
        CartesianProduct(rfns, tfns, rfns2);
        rfns.swap(rfns2);
      }
    }

    for (typename std::vector<AnnotatedFNS>::const_iterator q = rfns.begin();
         q != rfns.end(); ++q) {
      MatchItem newItem;
      newItem.annotatedFNS.fns = q->fns;
      newItem.annotatedFNS.fragment = item.annotatedFNS.fragment;
      newItem.annotatedFNS.fragment.insert(newItem.annotatedFNS.fragment.end(),
                                           q->fragment.begin(),
                                           q->fragment.end());
      newItem.trieNode = &child;
      m_queue.push(newItem);
    }
  }
}

template<typename Callback>
void RuleMatcherHyperTree<Callback>::CartesianProduct(
  const std::vector<AnnotatedFNS> &x,
  const std::vector<AnnotatedFNS> &y,
  std::vector<AnnotatedFNS> &z)
{
  z.clear();
  z.reserve(x.size() * y.size());
  for (typename std::vector<AnnotatedFNS>::const_iterator p = x.begin();
       p != x.end(); ++p) {
    const AnnotatedFNS &a = *p;
    for (typename std::vector<AnnotatedFNS>::const_iterator q = y.begin();
         q != y.end(); ++q) {
      const AnnotatedFNS &b = *q;
      // Create a new AnnotatedFNS.
      z.resize(z.size()+1);
      AnnotatedFNS &c = z.back();
      // Combine frontier node sequences from a and b.
      c.fns.reserve(a.fns.size() + b.fns.size());
      c.fns.assign(a.fns.begin(), a.fns.end());
      c.fns.insert(c.fns.end(), b.fns.begin(), b.fns.end());
      // Combine tree fragments from a and b.
      c.fragment.reserve(a.fragment.size() + b.fragment.size());
      c.fragment.assign(a.fragment.begin(), a.fragment.end());
      c.fragment.insert(c.fragment.end(), b.fragment.begin(), b.fragment.end());
    }
  }
}

template<typename Callback>
bool RuleMatcherHyperTree<Callback>::MatchChildren(
  const std::vector<Forest::Vertex *> &children,
  const HyperPath::NodeSeq &edgeLabel,
  std::size_t pos,
  std::size_t subSeqSize)
{
  if (children.size() != subSeqSize) {
    return false;
  }
  for (size_t i = 0; i < subSeqSize; ++i) {
    if (edgeLabel[pos+i] != children[i]->pvertex.symbol[0]->GetId()) {
      return false;
    }
  }
  return true;
}

template<typename Callback>
int RuleMatcherHyperTree<Callback>::CountCommas(const HyperPath::NodeSeq &seq)
{
  int count = 0;
  for (std::vector<std::size_t>::const_iterator p = seq.begin();
       p != seq.end(); ++p) {
    if (*p == HyperPath::kComma) {
      ++count;
    }
  }
  return count;
}

template<typename Callback>
int RuleMatcherHyperTree<Callback>::SubSeqLength(const HyperPath::NodeSeq &seq,
    int pos)
{
  int length = 0;
  HyperPath::NodeSeq::size_type curpos = pos;
  while (curpos != seq.size() && seq[curpos] != HyperPath::kComma) {
    ++curpos;
    ++length;
  }
  return length;
}

}  // namespace F2S
}  // namespace Syntax
}  // namespace Moses
