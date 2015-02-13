#pragma once

#include "moses/DecodeGraph.h"
#include "moses/ForestInput.h"
#include "moses/StaticData.h"
#include "moses/Syntax/BoundedPriorityContainer.h"
#include "moses/Syntax/CubeQueue.h"
#include "moses/Syntax/PHyperedge.h"
#include "moses/Syntax/RuleTable.h"
#include "moses/Syntax/RuleTableFF.h"
#include "moses/Syntax/SHyperedgeBundle.h"
#include "moses/Syntax/SVertex.h"
#include "moses/Syntax/SVertexRecombinationOrderer.h"
#include "moses/Syntax/SymbolEqualityPred.h"
#include "moses/Syntax/SymbolHasher.h"
#include "moses/Syntax/T2S/InputTree.h"
#include "moses/Syntax/T2S/InputTreeBuilder.h"
#include "moses/Syntax/T2S/InputTreeToForest.h"
#include "moses/TreeInput.h"

#include "DerivationWriter.h"
#include "GlueRuleSynthesizer.h"
#include "HyperTree.h"
#include "RuleMatcherCallback.h"
#include "TopologicalSorter.h"

namespace Moses
{
namespace Syntax
{
namespace F2S
{

template<typename RuleMatcher>
Manager<RuleMatcher>::Manager(const InputType &source)
    : Syntax::Manager(source)
{
  if (const ForestInput *p = dynamic_cast<const ForestInput*>(&source)) {
    m_forest = p->GetForest();
    m_rootVertex = p->GetRootVertex();
  } else if (const TreeInput *p = dynamic_cast<const TreeInput*>(&source)) {
    T2S::InputTreeBuilder builder;
    T2S::InputTree tmpTree;
    builder.Build(*p, "Q", tmpTree);
    boost::shared_ptr<Forest> forest = boost::make_shared<Forest>();
    m_rootVertex = T2S::InputTreeToForest(tmpTree, *forest);
    m_forest = forest;
  }
}

template<typename RuleMatcher>
void Manager<RuleMatcher>::Decode()
{
  const StaticData &staticData = StaticData::Instance();

  // Get various pruning-related constants.
  const std::size_t popLimit = staticData.GetCubePruningPopLimit();
  const std::size_t ruleLimit = staticData.GetRuleLimit();
  const std::size_t stackLimit = staticData.GetMaxHypoStackSize();

  // Initialize the stacks.
  InitializeStacks();

  // Initialize the rule matchers.
  InitializeRuleMatchers();

  // Create a callback to process the PHyperedges produced by the rule matchers.
  RuleMatcherCallback callback(m_stackMap, ruleLimit);

  // Create a glue rule synthesizer.
  GlueRuleSynthesizer glueRuleSynthesizer(*m_glueRuleTrie);

  // Sort the input forest's vertices into bottom-up topological order.
  std::vector<const Forest::Vertex *> sortedVertices;
  TopologicalSorter sorter;
  sorter.Sort(*m_forest, sortedVertices);

  // Visit each vertex of the input forest in topological order.
  for (std::vector<const Forest::Vertex *>::const_iterator
       p = sortedVertices.begin(); p != sortedVertices.end(); ++p) {
    const Forest::Vertex &vertex = **p;

    // Skip terminal vertices.
    if (vertex.incoming.empty()) {
      continue;
    }

    // Call the rule matchers to generate PHyperedges for this vertex and
    // convert each one to a SHyperedgeBundle (via the callback).  The
    // callback prunes the SHyperedgeBundles and keeps the best ones (up
    // to ruleLimit).
    callback.ClearContainer();
    for (typename std::vector<boost::shared_ptr<RuleMatcher> >::iterator
         q = m_mainRuleMatchers.begin(); q != m_mainRuleMatchers.end(); ++q) {
      (*q)->EnumerateHyperedges(vertex, callback);
    }

    // Retrieve the (pruned) set of SHyperedgeBundles from the callback.
    const BoundedPriorityContainer<SHyperedgeBundle> &bundles =
        callback.GetContainer();

    // Check if any rules were matched.  If not then for each incoming
    // hyperedge, synthesize a glue rule that is guaranteed to match.
    if (bundles.Size() == 0) {
      for (std::vector<Forest::Hyperedge *>::const_iterator p =
           vertex.incoming.begin(); p != vertex.incoming.end(); ++p) {
        glueRuleSynthesizer.SynthesizeRule(**p);
      }
      m_glueRuleMatcher->EnumerateHyperedges(vertex, callback);
      // FIXME This assertion occasionally fails -- why?
      // assert(bundles.Size() == vertex.incoming.size());
    }

    // Use cube pruning to extract SHyperedges from SHyperedgeBundles and
    // collect the SHyperedges in a buffer.
    CubeQueue cubeQueue(bundles.Begin(), bundles.End());
    std::size_t count = 0;
    std::vector<SHyperedge*> buffer;
    while (count < popLimit && !cubeQueue.IsEmpty()) {
      SHyperedge *hyperedge = cubeQueue.Pop();
      // FIXME See corresponding code in S2T::Manager
      // BEGIN{HACK}
      hyperedge->head->pvertex = &(vertex.pvertex);
      // END{HACK}
      buffer.push_back(hyperedge);
      ++count;
    }

    // Recombine SVertices and sort into a stack.
    SVertexStack &stack = m_stackMap[&(vertex.pvertex)];
    RecombineAndSort(buffer, stack);

    // Prune stack.
    if (stackLimit > 0 && stack.size() > stackLimit) {
      stack.resize(stackLimit);
    }
  }
}

template<typename RuleMatcher>
void Manager<RuleMatcher>::InitializeRuleMatchers()
{
  const std::vector<RuleTableFF*> &ffs = RuleTableFF::Instances();
  for (std::size_t i = 0; i < ffs.size(); ++i) {
    RuleTableFF *ff = ffs[i];
    // This may change in the future, but currently we assume that every
    // RuleTableFF is associated with a static, file-based rule table of
    // some sort and that the table should have been loaded into a RuleTable
    // by this point.
    const RuleTable *table = ff->GetTable();
    assert(table);
    RuleTable *nonConstTable = const_cast<RuleTable*>(table);
    HyperTree *trie = dynamic_cast<HyperTree*>(nonConstTable);
    assert(trie);
    boost::shared_ptr<RuleMatcher> p(new RuleMatcher(*trie));
    m_mainRuleMatchers.push_back(p);
  }

  // Create an additional rule trie + matcher for glue rules (which are
  // synthesized on demand).
  // FIXME Add a hidden RuleTableFF for the glue rule trie(?)
  m_glueRuleTrie.reset(new HyperTree(ffs[0]));
  m_glueRuleMatcher = boost::shared_ptr<RuleMatcher>(
    new RuleMatcher(*m_glueRuleTrie));
}

template<typename RuleMatcher>
void Manager<RuleMatcher>::InitializeStacks()
{
  // Check that m_forest has been initialized.
  assert(!m_forest->vertices.empty());

  for (std::vector<Forest::Vertex *>::const_iterator
       p = m_forest->vertices.begin(); p != m_forest->vertices.end(); ++p) {
    const Forest::Vertex &vertex = **p;

    // Create an empty stack.
    SVertexStack &stack = m_stackMap[&(vertex.pvertex)];

    // For terminals only, add a single SVertex.
    if (vertex.incoming.empty()) {
      boost::shared_ptr<SVertex> v(new SVertex());
      v->best = 0;
      v->pvertex = &(vertex.pvertex);
      stack.push_back(v);
    }
  }
}


template<typename RuleMatcher>
const SHyperedge *Manager<RuleMatcher>::GetBestSHyperedge() const
{
  PVertexToStackMap::const_iterator p = m_stackMap.find(&m_rootVertex->pvertex);
  assert(p != m_stackMap.end());
  const SVertexStack &stack = p->second;
  assert(!stack.empty());
  return stack[0]->best;
}

template<typename RuleMatcher>
void Manager<RuleMatcher>::ExtractKBest(
    std::size_t k,
    std::vector<boost::shared_ptr<KBestExtractor::Derivation> > &kBestList,
    bool onlyDistinct) const
{
  kBestList.clear();
  if (k == 0 || m_source.GetSize() == 0) {
    return;
  }

  // Get the top-level SVertex stack.
  PVertexToStackMap::const_iterator p = m_stackMap.find(&m_rootVertex->pvertex);
  assert(p != m_stackMap.end());
  const SVertexStack &stack = p->second;
  assert(!stack.empty());

  KBestExtractor extractor;

  if (!onlyDistinct) {
    // Return the k-best list as is, including duplicate translations.
    extractor.Extract(stack, k, kBestList);
    return;
  }

  // Determine how many derivations to extract.  If the k-best list is
  // restricted to distinct translations then this limit should be bigger
  // than k.  The k-best factor determines how much bigger the limit should be,
  // with 0 being 'unlimited.'  This actually sets a large-ish limit in case
  // too many translations are identical.
  const StaticData &staticData = StaticData::Instance();
  const std::size_t nBestFactor = staticData.GetNBestFactor();
  std::size_t numDerivations = (nBestFactor == 0) ? k*1000 : k*nBestFactor;

  // Extract the derivations.
  KBestExtractor::KBestVec bigList;
  bigList.reserve(numDerivations);
  extractor.Extract(stack, numDerivations, bigList);

  // Copy derivations into kBestList, skipping ones with repeated translations.
  std::set<Phrase> distinct;
  for (KBestExtractor::KBestVec::const_iterator p = bigList.begin();
       kBestList.size() < k && p != bigList.end(); ++p) {
    boost::shared_ptr<KBestExtractor::Derivation> derivation = *p;
    Phrase translation = KBestExtractor::GetOutputPhrase(*derivation);
    if (distinct.insert(translation).second) {
      kBestList.push_back(derivation);
    }
  }
}

// TODO Move this function into parent directory (Recombiner class?) and
// TODO share with S2T
template<typename RuleMatcher>
void Manager<RuleMatcher>::RecombineAndSort(
    const std::vector<SHyperedge*> &buffer, SVertexStack &stack)
{
  // Step 1: Create a map containing a single instance of each distinct vertex
  // (where distinctness is defined by the state value).  The hyperedges'
  // head pointers are updated to point to the vertex instances in the map and
  // any 'duplicate' vertices are deleted.
// TODO Set?
  typedef std::map<SVertex *, SVertex *, SVertexRecombinationOrderer> Map;
  Map map;
  for (std::vector<SHyperedge*>::const_iterator p = buffer.begin();
       p != buffer.end(); ++p) {
    SHyperedge *h = *p;
    SVertex *v = h->head;
    assert(v->best == h);
    assert(v->recombined.empty());
    std::pair<Map::iterator, bool> result = map.insert(Map::value_type(v, v));
    if (result.second) {
      continue;  // v's recombination value hasn't been seen before.
    }
    // v is a duplicate (according to the recombination rules).
    // Compare the score of h against the score of the best incoming hyperedge
    // for the stored vertex.
    SVertex *storedVertex = result.first->second;
    if (h->label.score > storedVertex->best->label.score) {
      // h's score is better.
      storedVertex->recombined.push_back(storedVertex->best);
      storedVertex->best = h;
    } else {
      storedVertex->recombined.push_back(h);
    }
    h->head->best = 0;
    delete h->head;
    h->head = storedVertex;
  }

  // Step 2: Copy the vertices from the map to the stack.
  stack.clear();
  stack.reserve(map.size());
  for (Map::const_iterator p = map.begin(); p != map.end(); ++p) {
    stack.push_back(boost::shared_ptr<SVertex>(p->first));
  }

  // Step 3: Sort the vertices in the stack.
  std::sort(stack.begin(), stack.end(), SVertexStackContentOrderer());
}

template<typename RuleMatcher>
void Manager<RuleMatcher>::OutputDetailedTranslationReport(
    OutputCollector *collector) const
{
  const SHyperedge *best = GetBestSHyperedge();
  if (best == NULL || collector == NULL) {
    return;
  }
  long translationId = m_source.GetTranslationId();
  std::ostringstream out;
  DerivationWriter::Write(*best, translationId, out);
  collector->Write(translationId, out.str());
}

}  // F2S
}  // Syntax
}  // Moses
