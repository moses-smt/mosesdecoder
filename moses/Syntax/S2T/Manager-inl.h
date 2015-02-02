#pragma once

#include <iostream>
#include <sstream>

#include "moses/DecodeGraph.h"
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

#include "DerivationWriter.h"
#include "OovHandler.h"
#include "PChart.h"
#include "RuleTrie.h"
#include "SChart.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

template<typename Parser>
Manager<Parser>::Manager(const InputType &source)
  : Syntax::Manager(source)
  , m_pchart(source.GetSize(), Parser::RequiresCompressedChart())
  , m_schart(source.GetSize())
{
}

template<typename Parser>
void Manager<Parser>::InitializeCharts()
{
  // Create a PVertex object and a SVertex object for each source word.
  for (std::size_t i = 0; i < m_source.GetSize(); ++i) {
    const Word &terminal = m_source.GetWord(i);

    // PVertex
    PVertex tmp(WordsRange(i,i), terminal);
    PVertex &pvertex = m_pchart.AddVertex(tmp);

    // SVertex
    boost::shared_ptr<SVertex> v(new SVertex());
    v->best = 0;
    v->pvertex = &pvertex;
    SChart::Cell &scell = m_schart.GetCell(i,i);
    SVertexStack stack(1, v);
    SChart::Cell::TMap::value_type x(terminal, stack);
    scell.terminalStacks.insert(x);
  }
}

template<typename Parser>
void Manager<Parser>::InitializeParsers(PChart &pchart,
                                        std::size_t ruleLimit)
{
  const std::vector<RuleTableFF*> &ffs = RuleTableFF::Instances();

  const std::vector<DecodeGraph*> &graphs =
    StaticData::Instance().GetDecodeGraphs();

  UTIL_THROW_IF2(ffs.size() != graphs.size(),
                 "number of RuleTables does not match number of decode graphs");

  for (std::size_t i = 0; i < ffs.size(); ++i) {
    RuleTableFF *ff = ffs[i];
    std::size_t maxChartSpan = graphs[i]->GetMaxChartSpan();
    // This may change in the future, but currently we assume that every
    // RuleTableFF is associated with a static, file-based rule table of
    // some sort and that the table should have been loaded into a RuleTable
    // by this point.
    const RuleTable *table = ff->GetTable();
    assert(table);
    RuleTable *nonConstTable = const_cast<RuleTable*>(table);
    boost::shared_ptr<Parser> parser;
    typename Parser::RuleTrie *trie =
      dynamic_cast<typename Parser::RuleTrie*>(nonConstTable);
    assert(trie);
    parser.reset(new Parser(pchart, *trie, maxChartSpan));
    m_parsers.push_back(parser);
  }

  // Check for OOVs and synthesize an additional rule trie + parser if
  // necessary.
  m_oovs.clear();
  std::size_t maxOovWidth = 0;
  FindOovs(pchart, m_oovs, maxOovWidth);
  if (!m_oovs.empty()) {
    // FIXME Add a hidden RuleTableFF for unknown words(?)
    OovHandler<typename Parser::RuleTrie> oovHandler(*ffs[0]);
    m_oovRuleTrie = oovHandler.SynthesizeRuleTrie(m_oovs.begin(), m_oovs.end());
    // Create a parser for the OOV rule trie.
    boost::shared_ptr<Parser> parser(
      new Parser(pchart, *m_oovRuleTrie, maxOovWidth));
    m_parsers.push_back(parser);
  }
}

// Find the set of OOVs for this input.  This function assumes that the
// PChart argument has already been initialized from the input.
template<typename Parser>
void Manager<Parser>::FindOovs(const PChart &pchart, std::set<Word> &oovs,
                               std::size_t maxOovWidth)
{
  // Get the set of RuleTries.
  std::vector<const RuleTrie *> tries;
  const std::vector<RuleTableFF*> &ffs = RuleTableFF::Instances();
  for (std::size_t i = 0; i < ffs.size(); ++i) {
    const RuleTableFF *ff = ffs[i];
    if (ff->GetTable()) {
      const RuleTrie *trie = dynamic_cast<const RuleTrie*>(ff->GetTable());
      assert(trie);  // FIXME
      tries.push_back(trie);
    }
  }

  // For every sink vertex in pchart (except for <s> and </s>), check whether
  // the word has a preterminal rule in any of the rule tables.  If not then
  // add it to the OOV set.
  oovs.clear();
  maxOovWidth = 0;
  // Assume <s> and </s> have been added at sentence boundaries, so skip
  // cells starting at position 0 and ending at the last position.
  for (std::size_t i = 1; i < pchart.GetWidth()-1; ++i) {
    for (std::size_t j = i; j < pchart.GetWidth()-1; ++j) {
      std::size_t width = j-i+1;
      const PChart::Cell::TMap &map = pchart.GetCell(i,j).terminalVertices;
      for (PChart::Cell::TMap::const_iterator p = map.begin();
           p != map.end(); ++p) {
        const Word &word = p->first;
        assert(!word.IsNonTerminal());
        bool found = false;
        for (std::vector<const RuleTrie *>::const_iterator q = tries.begin();
             q != tries.end(); ++q) {
          const RuleTrie *trie = *q;
          if (trie->HasPreterminalRule(word)) {
            found = true;
            break;
          }
        }
        if (!found) {
          oovs.insert(word);
          maxOovWidth = std::max(maxOovWidth, width);
        }
      }
    }
  }
}

template<typename Parser>
void Manager<Parser>::Decode()
{
  const StaticData &staticData = StaticData::Instance();

  // Get various pruning-related constants.
  const std::size_t popLimit = staticData.GetCubePruningPopLimit();
  const std::size_t ruleLimit = staticData.GetRuleLimit();
  const std::size_t stackLimit = staticData.GetMaxHypoStackSize();

  // Initialise the PChart and SChart.
  InitializeCharts();

  // Initialize the parsers.
  InitializeParsers(m_pchart, ruleLimit);

  // Create a callback to process the PHyperedges produced by the parsers.
  typename Parser::CallbackType callback(m_schart, ruleLimit);

  // Visit each cell of PChart in right-to-left depth-first order.
  std::size_t size = m_source.GetSize();
  for (int start = size-1; start >= 0; --start) {
    for (std::size_t width = 1; width <= size-start; ++width) {
      std::size_t end = start + width - 1;

      //PChart::Cell &pcell = m_pchart.GetCell(start, end);
      SChart::Cell &scell = m_schart.GetCell(start, end);

      WordsRange range(start, end);

      // Call the parsers to generate PHyperedges for this span and convert
      // each one to a SHyperedgeBundle (via the callback).  The callback
      // prunes the SHyperedgeBundles and keeps the best ones (up to ruleLimit).
      callback.InitForRange(range);
      for (typename std::vector<boost::shared_ptr<Parser> >::iterator
           p = m_parsers.begin(); p != m_parsers.end(); ++p) {
        (*p)->EnumerateHyperedges(range, callback);
      }

      // Retrieve the (pruned) set of SHyperedgeBundles from the callback.
      const BoundedPriorityContainer<SHyperedgeBundle> &bundles =
        callback.GetContainer();

      // Use cube pruning to extract SHyperedges from SHyperedgeBundles.
      // Collect the SHyperedges into buffers, one for each category.
      CubeQueue cubeQueue(bundles.Begin(), bundles.End());
      std::size_t count = 0;
      typedef boost::unordered_map<Word, std::vector<SHyperedge*>,
              SymbolHasher, SymbolEqualityPred > BufferMap;
      BufferMap buffers;
      while (count < popLimit && !cubeQueue.IsEmpty()) {
        SHyperedge *hyperedge = cubeQueue.Pop();
        // BEGIN{HACK}
        // The way things currently work, the LHS of each hyperedge is not
        // determined until just before the point of its creation, when a
        // target phrase is selected from the list of possible phrases (which
        // happens during cube pruning).  The cube pruning code doesn't (and
        // shouldn't) know about the contents of PChart and so creation of
        // the PVertex is deferred until this point.
        const Word &lhs = hyperedge->label.translation->GetTargetLHS();
        hyperedge->head->pvertex = &m_pchart.AddVertex(PVertex(range, lhs));
        // END{HACK}
        buffers[lhs].push_back(hyperedge);
        ++count;
      }

      // Recombine SVertices and sort into stacks.
      for (BufferMap::const_iterator p = buffers.begin(); p != buffers.end();
           ++p) {
        const Word &category = p->first;
        const std::vector<SHyperedge*> &buffer = p->second;
        std::pair<SChart::Cell::NMap::Iterator, bool> ret =
          scell.nonTerminalStacks.Insert(category, SVertexStack());
        assert(ret.second);
        SVertexStack &stack = ret.first->second;
        RecombineAndSort(buffer, stack);
      }

      // Prune stacks.
      if (stackLimit > 0) {
        for (SChart::Cell::NMap::Iterator p = scell.nonTerminalStacks.Begin();
             p != scell.nonTerminalStacks.End(); ++p) {
          SVertexStack &stack = p->second;
          if (stack.size() > stackLimit) {
            stack.resize(stackLimit);
          }
        }
      }

      // Prune the PChart cell for this span by removing vertices for
      // categories that don't occur in the SChart.
// Note: see HACK above.  Pruning the chart isn't currently necessary.
//      PrunePChart(scell, pcell);
    }
  }
}

template<typename Parser>
const SHyperedge *Manager<Parser>::GetBestSHyperedge() const
{
  const SChart::Cell &cell = m_schart.GetCell(0, m_source.GetSize()-1);
  const SChart::Cell::NMap &stacks = cell.nonTerminalStacks;
  if (stacks.Size() == 0) {
    return 0;
  }
  assert(stacks.Size() == 1);
  const std::vector<boost::shared_ptr<SVertex> > &stack = stacks.Begin()->second;
  // TODO Throw exception if stack is empty?  Or return 0?
  return stack[0]->best;
}

template<typename Parser>
void Manager<Parser>::ExtractKBest(
  std::size_t k,
  std::vector<boost::shared_ptr<KBestExtractor::Derivation> > &kBestList,
  bool onlyDistinct) const
{
  kBestList.clear();
  if (k == 0 || m_source.GetSize() == 0) {
    return;
  }

  // Get the top-level SVertex stack.
  const SChart::Cell &cell = m_schart.GetCell(0, m_source.GetSize()-1);
  const SChart::Cell::NMap &stacks = cell.nonTerminalStacks;
  if (stacks.Size() == 0) {
    return;
  }
  assert(stacks.Size() == 1);
  const std::vector<boost::shared_ptr<SVertex> > &stack = stacks.Begin()->second;
  // TODO Throw exception if stack is empty?  Or return 0?

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

template<typename Parser>
void Manager<Parser>::PrunePChart(const SChart::Cell &scell,
                                  PChart::Cell &pcell)
{
  /* FIXME
    PChart::Cell::VertexMap::iterator p = pcell.vertices.begin();
    while (p != pcell.vertices.end()) {
      const Word &category = p->first;
      if (scell.stacks.find(category) == scell.stacks.end()) {
        PChart::Cell::VertexMap::iterator q = p++;
        pcell.vertices.erase(q);
      } else {
        ++p;
      }
    }
  */
}

template<typename Parser>
void Manager<Parser>::RecombineAndSort(const std::vector<SHyperedge*> &buffer,
                                       SVertexStack &stack)
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

template<typename Parser>
void Manager<Parser>::OutputDetailedTranslationReport(
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

}  // S2T
}  // Syntax
}  // Moses
