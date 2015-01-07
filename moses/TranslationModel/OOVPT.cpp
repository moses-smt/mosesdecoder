// vim:tabstop=2
#include "OOVPT.h"
#include "moses/StaticData.h"
#include "moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerOOVPT.h"

using namespace std;

namespace Moses
{
OOVPT::OOVPT(const std::string &line)
  : PhraseDictionary(line)
{
  ReadParameters();
}

void OOVPT::Load()
{
  SetFeaturesToApply();
}

void OOVPT::InitializeForInput(InputType const& source)
{
  ReduceCache();
}

void OOVPT::GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const
{
  CacheColl &cache = GetCache();

  InputPathList::const_iterator iter;
  for (iter = inputPathQueue.begin(); iter != inputPathQueue.end(); ++iter) {
    InputPath &inputPath = **iter;
    const Phrase &sourcePhrase = inputPath.GetPhrase();
    const Word &sourceWord = sourcePhrase.GetWord(0);

    TargetPhrase *tp = CreateTargetPhrase(sourceWord);
    tp->EvaluateInIsolation(sourcePhrase);

    TargetPhraseCollection *tpColl = new TargetPhraseCollection();
    tpColl->Add(tp);

    // add target phrase to phrase-table cache
    size_t hash = hash_value(sourcePhrase);
    std::pair<const TargetPhraseCollection*, clock_t> value(tpColl, clock());
    cache[hash] = value;

    inputPath.SetTargetPhrases(*this, tpColl, NULL);
  }
}

TargetPhrase *OOVPT::CreateTargetPhrase(const Word &sourceWord) const
{
  // unknown word, add as trans opt
  const StaticData &staticData = StaticData::Instance();
  FactorCollection &factorCollection = FactorCollection::Instance();

  size_t isDigit = 0;

  const Factor *f = sourceWord[0]; // TODO hack. shouldn't know which factor is surface
  const StringPiece s = f->GetString();
  bool isEpsilon = (s=="" || s==EPSILON);
  if (staticData.GetDropUnknown()) {
	isDigit = s.find_first_of("0123456789");
	if (isDigit == string::npos)
	  isDigit = 0;
	else
	  isDigit = 1;
	// modify the starting bitmap
  }

  TargetPhrase *targetPhrase = new TargetPhrase(this);

  if (!(staticData.GetDropUnknown() || isEpsilon) || isDigit) {
	// add to dictionary

	Word &targetWord = targetPhrase->AddWord();
	targetWord.SetIsOOV(true);

	for (unsigned int currFactor = 0 ; currFactor < MAX_NUM_FACTORS ; currFactor++) {
	  FactorType factorType = static_cast<FactorType>(currFactor);

	  const Factor *sourceFactor = sourceWord[currFactor];
	  if (sourceFactor == NULL)
		targetWord[factorType] = factorCollection.AddFactor(UNKNOWN_FACTOR);
	  else
		targetWord[factorType] = factorCollection.AddFactor(sourceFactor->GetString());
	}
	//create a one-to-one alignment between UNKNOWN_FACTOR and its verbatim translation

	targetPhrase->SetAlignmentInfo("0-0");

  } else {
	// drop source word. create blank target phrase
  }

  float unknownScore = FloorScore(TransformScore(0));
  targetPhrase->GetScoreBreakdown().Assign(this, unknownScore);

  return targetPhrase;
}

ChartRuleLookupManager* OOVPT::CreateRuleLookupManager(const ChartParser &parser,
    const ChartCellCollectionBase &cellCollection,
    std::size_t /*maxChartSpan*/)
{
  return new ChartRuleLookupManagerOOVPT(parser, cellCollection, *this);
}

TO_STRING_BODY(OOVPT);

// friend
ostream& operator<<(ostream& out, const OOVPT& phraseDict)
{
  return out;
}

}
