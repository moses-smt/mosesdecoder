// vim:tabstop=2
#include "SkeletonPT.h"
#include "moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerSkeleton.h"

using namespace std;

namespace Moses
{
SkeletonPT::SkeletonPT(const std::string &line)
  : PhraseDictionary(line)
{
  ReadParameters();
}

void SkeletonPT::Load()
{
  SetFeaturesToApply();
}

void SkeletonPT::InitializeForInput(InputType const& source)
{
  ReduceCache();
}

void SkeletonPT::GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const
{
  CacheColl &cache = GetCache();

  InputPathList::const_iterator iter;
  for (iter = inputPathQueue.begin(); iter != inputPathQueue.end(); ++iter) {
    InputPath &inputPath = **iter;
    const Phrase &sourcePhrase = inputPath.GetPhrase();

    TargetPhrase *tp = CreateTargetPhrase(sourcePhrase);
    TargetPhraseCollection *tpColl = new TargetPhraseCollection();
    tpColl->Add(tp);

    // add target phrase to phrase-table cache
    size_t hash = hash_value(sourcePhrase);
    std::pair<const TargetPhraseCollection*, clock_t> value(tpColl, clock());
    cache[hash] = value;

    inputPath.SetTargetPhrases(*this, tpColl, NULL);
  }
}

TargetPhrase *SkeletonPT::CreateTargetPhrase(const Phrase &sourcePhrase) const
{
  // create a target phrase from the 1st word of the source, prefix with 'SkeletonPT:'
  assert(sourcePhrase.GetSize());
  assert(m_output.size() == 1);

  string str = sourcePhrase.GetWord(0).GetFactor(0)->GetString().as_string();
  str = "SkeletonPT:" + str;

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

ChartRuleLookupManager* SkeletonPT::CreateRuleLookupManager(const ChartParser &parser,
    const ChartCellCollectionBase &cellCollection,
    std::size_t /*maxChartSpan*/)
{
  return new ChartRuleLookupManagerSkeleton(parser, cellCollection, *this);
}

TO_STRING_BODY(SkeletonPT);

// friend
ostream& operator<<(ostream& out, const SkeletonPT& phraseDict)
{
  return out;
}

}
