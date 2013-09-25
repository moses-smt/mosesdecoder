// vim:tabstop=2
#include "SkeletonPT.h"

using namespace std;

namespace Moses
{
SkeletonPT::SkeletonPT(const std::string &line)
  : PhraseDictionary("SkeletonPT", line)
{
  ReadParameters();
}

void SkeletonPT::CleanUpAfterSentenceProcessing(const InputType& source)
{
	RemoveAllInColl(m_allTPColl);
}

void SkeletonPT::GetTargetPhraseCollectionBatch(const InputPathList &phraseDictionaryQueue) const
{
  InputPathList::const_iterator iter;
  for (iter = phraseDictionaryQueue.begin(); iter != phraseDictionaryQueue.end(); ++iter) {
    InputPath &inputPath = **iter;

    TargetPhrase *tp = CreateTargetPhrase(inputPath.GetPhrase());
    TargetPhraseCollection *tpColl = new TargetPhraseCollection();
    tpColl->Add(tp);

    m_allTPColl.push_back(tpColl);
    inputPath.SetTargetPhrases(*this, tpColl, NULL);
  }
}

TargetPhrase *SkeletonPT::CreateTargetPhrase(const Phrase &sourcePhrase) const
{
	CHECK(sourcePhrase.GetSize());

	TargetPhrase *tp = new TargetPhrase();
	tp->AddWord(sourcePhrase.GetWord(0));

	// score for this phrase table
	vector<float> scores(m_numScoreComponents, 1.3);
	tp->GetScoreBreakdown().PlusEquals(this, scores);

	// score of all other ff when this rule is being loaded
	tp->Evaluate(sourcePhrase, GetFeaturesToApply());

	return tp;
}

ChartRuleLookupManager* SkeletonPT::CreateRuleLookupManager(const ChartParser&,
															const ChartCellCollectionBase&)
{
  CHECK(false);
}

TO_STRING_BODY(SkeletonPT);

// friend
ostream& operator<<(ostream& out, const SkeletonPT& phraseDict)
{
  return out;
}

}
