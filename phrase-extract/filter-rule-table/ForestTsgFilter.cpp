#include "ForestTsgFilter.h"

#include <boost/make_shared.hpp>

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

// kMatchLimit is used to limit the effort spent trying to match an individual
// rule.  It defines the maximum number of times that MatchFragment() can be
// called before the search is aborted and the rule is (possibly wrongly)
// accepted.
// FIXME Use a better matching algorithm.
const std::size_t ForestTsgFilter::kMatchLimit = 10000;

ForestTsgFilter::ForestTsgFilter(
  const std::vector<boost::shared_ptr<StringForest> > &sentences)
{
  // Convert each StringForest to an IdForest.
  m_sentences.reserve(sentences.size());
  for (std::vector<boost::shared_ptr<StringForest> >::const_iterator p =
         sentences.begin(); p != sentences.end(); ++p) {
    m_sentences.push_back(StringForestToIdForest(**p));
  }

  // Construct a map from vocabulary Ids to IdForest nodes.
  m_idToSentence.resize(m_testVocab.Size());
  for (std::size_t i = 0; i < m_sentences.size(); ++i) {
    const IdForest &forest = *(m_sentences[i]);
    for (std::vector<IdForest::Vertex *>::const_iterator
         p = forest.vertices.begin(); p != forest.vertices.end(); ++p) {
      m_idToSentence[(*p)->value.id][i].push_back(*p);
    }
  }
}

boost::shared_ptr<ForestTsgFilter::IdForest>
ForestTsgFilter::StringForestToIdForest(const StringForest &f)
{
  typedef StringForest::Vertex StringVertex;
  typedef StringForest::Hyperedge StringHyperedge;
  typedef IdForest::Vertex IdVertex;
  typedef IdForest::Hyperedge IdHyperedge;

  boost::shared_ptr<IdForest> g = boost::make_shared<IdForest>();

  // Map from f's vertices to g's vertices.
  boost::unordered_map<const StringVertex *, const IdVertex *> vertexMap;

  // Create idForest's vertices and populate vertexMap.
  for (std::vector<StringVertex *>::const_iterator p = f.vertices.begin();
       p != f.vertices.end(); ++p) {
    const StringVertex *v = *p;
    IdVertex *w = new IdVertex();
    w->value.id = m_testVocab.Insert(v->value.symbol);
    w->value.start = v->value.start;
    w->value.end = v->value.end;
    g->vertices.push_back(w);
    vertexMap[v] = w;
  }

  // Create g's hyperedges.
  for (std::vector<StringVertex *>::const_iterator p = f.vertices.begin();
       p != f.vertices.end(); ++p) {
    for (std::vector<StringHyperedge *>::const_iterator
         q = (*p)->incoming.begin(); q != (*p)->incoming.end(); ++q) {
      IdHyperedge *e = new IdHyperedge();
      e->head = const_cast<IdVertex *>(vertexMap[(*q)->head]);
      e->tail.reserve((*q)->tail.size());
      for (std::vector<StringVertex*>::const_iterator
           r = (*q)->tail.begin(); r != (*q)->tail.end(); ++r) {
        e->tail.push_back(const_cast<IdVertex *>(vertexMap[*r]));
      }
      e->head->incoming.push_back(e);
    }
  }

  return g;
}

bool ForestTsgFilter::MatchFragment(const IdTree &fragment,
                                    const std::vector<IdTree *> &leaves)
{
  typedef std::vector<const IdTree *> TreeVec;

  // Reset the match counter.
  m_matchCount = 0;

  // Determine which of the fragment's leaves occurs in the smallest number of
  // sentences in the test set.  If the fragment contains a rare word
  // (which is pretty likely assuming a Zipfian distribution) then we only
  // have to try matching the fragment against a small number of potential
  // match sites.
  const IdTree *rarestLeaf = leaves[0];
  std::size_t lowestCount = m_idToSentence[rarestLeaf->value()].size();
  for (std::size_t i = 1; i < leaves.size(); ++i) {
    const IdTree *leaf = leaves[i];
    std::size_t count = m_idToSentence[leaf->value()].size();
    if (count < lowestCount) {
      lowestCount = count;
      rarestLeaf = leaf;
    }
  }

  // Try to match the rule fragment against the sentences where the rarest
  // leaf was found.
  const InnerMap &leafSentenceMap = m_idToSentence[rarestLeaf->value()];
  const InnerMap &rootSentenceMap = m_idToSentence[fragment.value()];

  std::vector<std::pair<std::size_t, std::size_t> > spans;
  // For each forest i that contains the rarest leaf symbol...
  for (InnerMap::const_iterator p = leafSentenceMap.begin();
       p != leafSentenceMap.end(); ++p) {
    std::size_t i = p->first;
    // Get the set of candidate match sites in forest i (these are vertices
    // with the same label as the root of the rule fragment).
    InnerMap::const_iterator q = rootSentenceMap.find(i);
    if (q == rootSentenceMap.end()) {
      continue;
    }
    const std::vector<const IdForest::Vertex*> &candidates = q->second;
    // Record the span(s) of the rare leaf symbol in forest i.
    spans.clear();
    for (std::vector<const IdForest::Vertex*>::const_iterator
         r = p->second.begin(); r != p->second.end(); ++r) {
      spans.push_back(std::make_pair((*r)->value.start, (*r)->value.end));
    }
    // For each candidate match site in forest i...
    for (std::vector<const IdForest::Vertex*>::const_iterator
         r = candidates.begin(); r != candidates.end(); ++r) {
      const IdForest::Vertex &v = **r;
      // Check that the subtrees rooted at v are at least as wide as the
      // fragment (counting each non-terminal as being one token wide).
      if (v.value.end - v.value.start + 1 < leaves.size()) {
        continue;
      }
      // Check that the candidate's span covers one of the rare leaf symbols.
      bool covered = false;
      for (std::vector<std::pair<std::size_t, std::size_t> >::const_iterator
           s = spans.begin(); s != spans.end(); ++s) {
        if (v.value.start <= s->first && v.value.end >= s->second) {
          covered = true;
          break;
        }
      }
      if (!covered) {
        continue;
      }
      // Attempt to match the fragment at the candidate site.
      if (MatchFragment(fragment, v)) {
        return true;
      }
    }
  }
  return false;
}

bool ForestTsgFilter::MatchFragment(const IdTree &fragment,
                                    const IdForest::Vertex &v)
{
  if (++m_matchCount >= kMatchLimit) {
    return true;
  }
  if (fragment.value() != v.value.id) {
    return false;
  }
  const std::vector<IdTree*> &children = fragment.children();
  if (children.empty()) {
    return true;
  }
  for (std::vector<IdForest::Hyperedge *>::const_iterator
       p = v.incoming.begin(); p != v.incoming.end(); ++p) {
    const std::vector<IdForest::Vertex*> &tail = (*p)->tail;
    if (children.size() != tail.size()) {
      continue;
    }
    bool match = true;
    for (std::size_t i = 0; i < children.size(); ++i) {
      if (!MatchFragment(*children[i], *tail[i])) {
        match = false;
        break;
      }
    }
    if (match) {
      return true;
    }
  }
  return false;
}

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining
