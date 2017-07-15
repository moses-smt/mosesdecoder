// vim:tabstop=2
#include "ExamplePT.h"
#include "moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerExample.h"

using namespace std;

namespace Moses
{
ExamplePT::ExamplePT(const std::string &line)
  : PhraseDictionary(line, true)
{
  ReadParameters();
}

void ExamplePT::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  SetFeaturesToApply();
}

void ExamplePT::InitializeForInput(ttasksptr const& ttask)
{
  ReduceCache();
}

void ExamplePT::GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const
{
  CacheColl &cache = GetCache();

  InputPathList::const_iterator iter;
  for (iter = inputPathQueue.begin(); iter != inputPathQueue.end(); ++iter) {
    InputPath &inputPath = **iter;
    const Phrase &sourcePhrase = inputPath.GetPhrase();

    TargetPhrase *tp = CreateTargetPhrase(sourcePhrase);
    TargetPhraseCollection::shared_ptr tpColl(new TargetPhraseCollection);
    tpColl->Add(tp);

    // add target phrase to phrase-table cache
    size_t hash = hash_value(sourcePhrase);
    std::pair<TargetPhraseCollection::shared_ptr, clock_t>
    value(tpColl, clock());
    cache[hash] = value;

    inputPath.SetTargetPhrases(*this, tpColl, NULL);
  }
}

TargetPhrase *ExamplePT::CreateTargetPhrase(const Phrase &sourcePhrase) const
{
  // create a target phrase from the 1st word of the source, prefix with 'ExamplePT:'
  assert(sourcePhrase.GetSize());
  assert(m_output.size() == 1);

  string str = sourcePhrase.GetWord(0).GetFactor(0)->GetString().as_string();
  str = "ExamplePT:" + str;

  TargetPhrase *tp = new TargetPhrase(this);
  Word &word = tp->AddWord();
  word.CreateFromString(Output, m_output, str, false);

  // score for this phrase table
  vector<float> scores(m_numScoreComponents, 1.3);
  tp->GetScoreBreakdown().PlusEquals(this, scores);

  // score of all other ff when this rule is being loaded
  tp->EvaluateInIsolation(sourcePhrase, GetFeaturesToApply());

  return tp;
}

ChartRuleLookupManager* ExamplePT::CreateRuleLookupManager(const ChartParser &parser,
    const ChartCellCollectionBase &cellCollection,
    std::size_t /*maxChartSpan*/)
{
  return new ChartRuleLookupManagerExample(parser, cellCollection, *this);
}

TO_STRING_BODY(ExamplePT);

// friend
ostream& operator<<(ostream& out, const ExamplePT& phraseDict)
{
  return out;
}

}
