#include "RuleTableFF.h"

#include "moses/StaticData.h"
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
  : PhraseDictionary(line)
{
  ReadParameters();
  // caching for memory pt is pointless
  m_maxCacheSize = 0;

  s_instances.push_back(this);
}

void RuleTableFF::Load()
{
  SetFeaturesToApply();

  const StaticData &staticData = StaticData::Instance();
  if (staticData.GetSearchAlgorithm() == SyntaxF2S ||
      staticData.GetSearchAlgorithm() == SyntaxT2S) {
    F2S::HyperTree *trie = new F2S::HyperTree(this);
    F2S::HyperTreeLoader loader;
    loader.Load(m_input, m_output, m_filePath, *this, *trie);
    m_table = trie;
  } else if (staticData.GetSearchAlgorithm() == SyntaxS2T) {
    S2TParsingAlgorithm algorithm = staticData.GetS2TParsingAlgorithm();
    if (algorithm == RecursiveCYKPlus) {
      S2T::RuleTrieCYKPlus *trie = new S2T::RuleTrieCYKPlus(this);
      S2T::RuleTrieLoader loader;
      loader.Load(m_input, m_output, m_filePath, *this, *trie);
      m_table = trie;
    } else if (algorithm == Scope3) {
      S2T::RuleTrieScope3 *trie = new S2T::RuleTrieScope3(this);
      S2T::RuleTrieLoader loader;
      loader.Load(m_input, m_output, m_filePath, *this, *trie);
      m_table = trie;
    } else {
      UTIL_THROW2("ERROR: unhandled S2T parsing algorithm");
    }
  } else if (staticData.GetSearchAlgorithm() == SyntaxT2S_SCFG) {
    T2S::RuleTrie *trie = new T2S::RuleTrie(this);
    T2S::RuleTrieLoader loader;
    loader.Load(m_input, m_output, m_filePath, *this, *trie);
    m_table = trie;
  } else {
    UTIL_THROW2(
      "ERROR: RuleTableFF currently only supports the S2T, T2S, T2S_SCFG, and F2S search algorithms");
  }
}

}  // Syntax
}  // Moses
