// vim:tabstop=2
#include "TransliterationPhraseDictionary.h"
#include "moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerSkeleton.h"

using namespace std;

namespace Moses
{
TransliterationPhraseDictionary::TransliterationPhraseDictionary(const std::string &line)
  : PhraseDictionary(line)
{
  ReadParameters();
}

void TransliterationPhraseDictionary::CleanUpAfterSentenceProcessing(const InputType& source)
{
  RemoveAllInColl(m_allTPColl);
}

void TransliterationPhraseDictionary::GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const
{
  InputPathList::const_iterator iter;
  for (iter = inputPathQueue.begin(); iter != inputPathQueue.end(); ++iter) {
    InputPath &inputPath = **iter;
    const Phrase &sourcePhrase = inputPath.GetPhrase();

    if (sourcePhrase.GetSize() != 1) {
    	// only translit single words. This should be user configurable
    	continue;
    }

    InputPath::TargetPhrases::const_iterator iter;
    for (iter = inputPath.GetTargetPhrases().begin(); iter != inputPath.GetTargetPhrases().end(); ++iter) {
    	const std::pair<const TargetPhraseCollection*, const void*> &temp = iter->second;
    	const TargetPhraseCollection *tpCollPrev = temp.first;

    	if (tpCollPrev && tpCollPrev->GetSize()) {
    		// already have translation from another pt. Don't transliterate
    		break;
    	}

    	// TRANSLITERATE
    	// /home/nadir/mosesdecoder/scripts/Transliteration/prepare-transliteration-phrase-table.pl --transliteration-model-dir /home/nadir/iwslt13-en-ar/model/Transliteration.3 --moses-src-dir /home/nadir/mosesdecoder --external-bin-dir /home/pkoehn/statmt/bin --input-extension en --output-extension ar --oov-file /fs/syn4/nadir/iwslt13-en-ar/evaluation/temp.oov --out-dir /home/nadir/iwslt13-en-ar/model/Transliteration-Phrase-Table.3
        TargetPhrase *tp = CreateTargetPhrase(sourcePhrase);
        TargetPhraseCollection *tpColl = new TargetPhraseCollection();
        tpColl->Add(tp);

        m_allTPColl.push_back(tpColl);
        inputPath.SetTargetPhrases(*this, tpColl, NULL);

    }

  }
}

TargetPhrase *TransliterationPhraseDictionary::CreateTargetPhrase(const Phrase &sourcePhrase) const
{
  // create a target phrase from the 1st word of the source, prefix with 'TransliterationPhraseDictionary:'
  assert(sourcePhrase.GetSize());
  assert(m_output.size() == 1);

  string str = sourcePhrase.GetWord(0).GetFactor(0)->GetString().as_string();
  str = "TransliterationPhraseDictionary:" + str;

  TargetPhrase *tp = new TargetPhrase();
  Word &word = tp->AddWord();
  word.CreateFromString(Output, m_output, str, false);

  // score for this phrase table
  vector<float> scores(m_numScoreComponents, 1.3);
  tp->GetScoreBreakdown().PlusEquals(this, scores);

  // score of all other ff when this rule is being loaded
  tp->Evaluate(sourcePhrase, GetFeaturesToApply());

  return tp;
}

ChartRuleLookupManager* TransliterationPhraseDictionary::CreateRuleLookupManager(const ChartParser &parser,
    const ChartCellCollectionBase &cellCollection)
{
	return NULL;
  //return new ChartRuleLookupManagerSkeleton(parser, cellCollection, *this);
}

TO_STRING_BODY(TransliterationPhraseDictionary);

// friend
ostream& operator<<(ostream& out, const TransliterationPhraseDictionary& phraseDict)
{
  return out;
}

}
