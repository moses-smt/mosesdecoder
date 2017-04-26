// vim:tabstop=2
#include "PhraseDictionaryMemoryPerSentence.h"
#include "moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerExample.h"

using namespace std;

namespace Moses
{
PhraseDictionaryMemoryPerSentence::PhraseDictionaryMemoryPerSentence(const std::string &line)
  : PhraseDictionary(line, true)
{
  ReadParameters();
}

void PhraseDictionaryMemoryPerSentence::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  SetFeaturesToApply();

  // don't load anything. Load when we have the input
}

void PhraseDictionaryMemoryPerSentence::InitializeForInput(ttasksptr const& ttask)
{
  Coll &coll = GetColl();
  coll.clear();

  string filePath = m_filePath + SPrint(ttask.get()->GetSource()->GetTranslationId()) + ".txt";
  InputFileStream strme(filePath);

  string line;
  while (getline(strme, line)) {
    vector<string> toks = TokenizeMultiCharSeparator(line, "|||");
    Phrase source;
    source.CreateFromString(Input, m_input, toks[0], NULL);

    TargetPhrase *target = new TargetPhrase(this);
    target->CreateFromString(Output, m_output, toks[1], NULL);

    // score for this phrase table
    vector<float> scores = Tokenize<float>(toks[2]);
    std::transform(scores.begin(), scores.end(), scores.begin(),TransformScore);
    std::transform(scores.begin(), scores.end(), scores.begin(),FloorScore);
    target->GetScoreBreakdown().PlusEquals(this, scores);

    // score of all other ff when this rule is being loaded
    target->EvaluateInIsolation(source, GetFeaturesToApply());

    // add to coll
    TargetPhraseCollection::shared_ptr &tpsPtr = coll[source];
    TargetPhraseCollection *tps = tpsPtr.get();
    if (tps == NULL) {
      tps = new TargetPhraseCollection();
      tpsPtr.reset(tps);
    }
    tps->Add(target);
  }
}

void PhraseDictionaryMemoryPerSentence::GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const
{
  InputPathList::const_iterator iter;
  for (iter = inputPathQueue.begin(); iter != inputPathQueue.end(); ++iter) {
    InputPath &inputPath = **iter;
    const Phrase &source = inputPath.GetPhrase();

    Coll &coll = GetColl();
    Coll::const_iterator iter = coll.find(source);
    if (iter == coll.end()) {
      TargetPhraseCollection::shared_ptr tprPtr;
      inputPath.SetTargetPhrases(*this, tprPtr, NULL);
    } else {
      const TargetPhraseCollection::shared_ptr &tprPtr = iter->second;
      inputPath.SetTargetPhrases(*this, tprPtr, NULL);
    }
  }
}


ChartRuleLookupManager* PhraseDictionaryMemoryPerSentence::CreateRuleLookupManager(const ChartParser &parser,
    const ChartCellCollectionBase &cellCollection,
    std::size_t /*maxChartSpan*/)
{
  abort();
}

PhraseDictionaryMemoryPerSentence::Coll &PhraseDictionaryMemoryPerSentence::GetColl() const
{
  Coll *coll;
  coll = m_coll.get();
  if (coll == NULL) {
    coll = new Coll;
    m_coll.reset(coll);
  }
  assert(coll);
  return *coll;
}

TO_STRING_BODY(PhraseDictionaryMemoryPerSentence);

// friend
ostream& operator<<(ostream& out, const PhraseDictionaryMemoryPerSentence& phraseDict)
{
  return out;
}

}
