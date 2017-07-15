// vim:tabstop=2
#include "PhraseDictionaryMemoryPerSentenceOnDemand.h"
#include <sstream>

using namespace std;

namespace Moses
{
PhraseDictionaryMemoryPerSentenceOnDemand::PhraseDictionaryMemoryPerSentenceOnDemand(const std::string &line)
  : PhraseDictionary(line, true), m_valuesAreProbabilities(true)
{
  ReadParameters();
}

void PhraseDictionaryMemoryPerSentenceOnDemand::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  SetFeaturesToApply();

  // don't load anything. Load when we have the input
}


TargetPhraseCollection::shared_ptr PhraseDictionaryMemoryPerSentenceOnDemand::GetTargetPhraseCollectionNonCacheLEGACY(const Phrase &source) const
{

  Coll &coll = GetColl();

  return coll[source];

}


void PhraseDictionaryMemoryPerSentenceOnDemand::InitializeForInput(ttasksptr const& ttask)
{
  Coll &coll = GetColl();
  coll.clear();

  VERBOSE(2, "Initializing PhraseDictionaryMemoryPerSentenceOnDemand " << m_description << "\n");

  // The context scope object for this translation task
  //     contains a map of translation task-specific data
  boost::shared_ptr<Moses::ContextScope> contextScope = ttask->GetScope();

  // The key to the map is this object
  void const* key = static_cast<void const*>(this);

  // The value stored in the map is a string representing a phrase table
  boost::shared_ptr<string> value = contextScope->get<string>(key);

  // Create a stream to read the phrase table data
  stringstream strme(*(value.get()));

  // Read the phrase table data, one line at a time
  string line;
  while (getline(strme, line)) {

    VERBOSE(3, "\t" << line);

    vector<string> toks = TokenizeMultiCharSeparator(line, "|||");
    Phrase source;
    source.CreateFromString(Input, m_input, toks[0], NULL);

    TargetPhrase *target = new TargetPhrase(this);
    target->CreateFromString(Output, m_output, toks[1], NULL);

    // score for this phrase table
    vector<float> scores = Tokenize<float>(toks[2]);
    if (m_valuesAreProbabilities) {
      std::transform(scores.begin(), scores.end(), scores.begin(),TransformScore);
      std::transform(scores.begin(), scores.end(), scores.begin(),FloorScore);
    }
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

void PhraseDictionaryMemoryPerSentenceOnDemand::GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const
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


ChartRuleLookupManager* PhraseDictionaryMemoryPerSentenceOnDemand::CreateRuleLookupManager(const ChartParser &parser,
    const ChartCellCollectionBase &cellCollection,
    std::size_t /*maxChartSpan*/)
{
  abort();
}

PhraseDictionaryMemoryPerSentenceOnDemand::Coll &PhraseDictionaryMemoryPerSentenceOnDemand::GetColl() const
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

void
PhraseDictionaryMemoryPerSentenceOnDemand::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
    UTIL_THROW(util::Exception, "PhraseDictionaryMemoryPerSentenceOnDemand does not support key \"path\".");
  } else if (key == "valuesAreProbabilities") {
    m_valuesAreProbabilities = Scan<bool>(value);
  } else {
    PhraseDictionary::SetParameter(key, value);
  }
}


TO_STRING_BODY(PhraseDictionaryMemoryPerSentenceOnDemand);

// friend
ostream& operator<<(ostream& out, const PhraseDictionaryMemoryPerSentenceOnDemand& phraseDict)
{
  return out;
}

}
