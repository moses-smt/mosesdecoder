#include "RuleTableFF.h"
#include "moses/parameters/AllOptions.h"
#include "moses/Syntax/F2S/HyperTree.h"
#include "moses/Syntax/F2S/HyperTreeLoader.h"
#include "moses/Syntax/S2T/RuleTrieCYKPlus.h"
#include "moses/Syntax/S2T/RuleTrieLoader.h"
#include "moses/Syntax/S2T/RuleTrieScope3.h"
#include "moses/Syntax/T2S/RuleTrie.h"
#include "moses/Syntax/T2S/RuleTrieLoader.h"
namespace Moses
{
namespace Syntax
{

std::vector<RuleTableFF*> RuleTableFF::s_instances;

RuleTableFF::RuleTableFF(const std::string &line)
  : PhraseDictionary(line, true)
{
  ReadParameters();
  // caching for memory pt is pointless
  m_maxCacheSize = 0;

  s_instances.push_back(this);
}

void RuleTableFF::Load(Moses::AllOptions::ptr const& opts)
{
  m_options = opts;
  SetFeaturesToApply();

  if (opts->search.algo == SyntaxF2S || opts->search.algo == SyntaxT2S) {
    F2S::HyperTree *trie = new F2S::HyperTree(this);
    F2S::HyperTreeLoader loader;
    loader.Load(*opts, m_input, m_output, m_filePath, *this, *trie, m_sourceTerminalSet);
    m_table = trie;
  } else if (opts->search.algo == SyntaxS2T) {
    S2TParsingAlgorithm algorithm = opts->syntax.s2t_parsing_algo; // staticData.GetS2TParsingAlgorithm();
    if (algorithm == RecursiveCYKPlus) {
      S2T::RuleTrieCYKPlus *trie = new S2T::RuleTrieCYKPlus(this);
      S2T::RuleTrieLoader loader;
      loader.Load(*opts,m_input, m_output, m_filePath, *this, *trie);
      m_table = trie;
    } else if (algorithm == Scope3) {
      S2T::RuleTrieScope3 *trie = new S2T::RuleTrieScope3(this);
      S2T::RuleTrieLoader loader;
      loader.Load(*opts, m_input, m_output, m_filePath, *this, *trie);
      m_table = trie;
    } else {
      UTIL_THROW2("ERROR: unhandled S2T parsing algorithm");
    }
  } else if (opts->search.algo == SyntaxT2S_SCFG) {
    T2S::RuleTrie *trie = new T2S::RuleTrie(this);
    T2S::RuleTrieLoader loader;
    loader.Load(*opts, m_input, m_output, m_filePath, *this, *trie);
    m_table = trie;
  } else {
    UTIL_THROW2(
      "ERROR: RuleTableFF currently only supports the S2T, T2S, T2S_SCFG, and F2S search algorithms");
  }
}

}  // Syntax
}  // Moses
