// vim:tabstop=2
#include <cstdlib>

#include "PhraseDictionaryTransliteration.h"
#include "moses/DecodeGraph.h"
#include "moses/DecodeStep.h"
#include "util/tempfile.hh"

using namespace std;

namespace Moses
{
PhraseDictionaryTransliteration::PhraseDictionaryTransliteration(const std::string &line)
  : PhraseDictionary(line, true)
{
  ReadParameters();
  UTIL_THROW_IF2(m_mosesDir.empty() ||
                 m_scriptDir.empty() ||
                 m_externalDir.empty() ||
                 m_inputLang.empty() ||
                 m_outputLang.empty(), "Must specify all arguments");
}

void PhraseDictionaryTransliteration::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  SetFeaturesToApply();
}

void PhraseDictionaryTransliteration::CleanUpAfterSentenceProcessing(const InputType& source)
{
  ReduceCache();
}

void PhraseDictionaryTransliteration::GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const
{

  InputPathList::const_iterator iter;
  for (iter = inputPathQueue.begin(); iter != inputPathQueue.end(); ++iter) {
    InputPath &inputPath = **iter;

    if (!SatisfyBackoff(inputPath)) {
      continue;
    }

    const Phrase &sourcePhrase = inputPath.GetPhrase();

    if (sourcePhrase.GetSize() != 1) {
      // only translit single words. A limitation of the translit script
      continue;
    }

    GetTargetPhraseCollection(inputPath);
  }
}

void
PhraseDictionaryTransliteration::
GetTargetPhraseCollection(InputPath &inputPath) const
{
  const Phrase &sourcePhrase = inputPath.GetPhrase();
  size_t hash = hash_value(sourcePhrase);

  CacheColl &cache = GetCache();

  CacheColl::iterator iter;
  iter = cache.find(hash);

  if (iter != cache.end()) {
    // already in cache
    TargetPhraseCollection::shared_ptr tpColl = iter->second.first;
    inputPath.SetTargetPhrases(*this, tpColl, NULL);
  } else {
    // TRANSLITERATE
    const util::temp_file inFile;
    const util::temp_dir outDir;

    ofstream inStream(inFile.path().c_str());
    inStream << sourcePhrase.ToString() << endl;
    inStream.close();

    string cmd = m_scriptDir + "/Transliteration/prepare-transliteration-phrase-table.pl" +
                 " --transliteration-model-dir " + m_filePath +
                 " --moses-src-dir " + m_mosesDir +
                 " --external-bin-dir " + m_externalDir +
                 " --input-extension " + m_inputLang +
                 " --output-extension " + m_outputLang +
                 " --oov-file " + inFile.path() +
                 " --out-dir " + outDir.path();

    int ret = system(cmd.c_str());
    UTIL_THROW_IF2(ret != 0, "Transliteration script error");

    TargetPhraseCollection::shared_ptr tpColl(new TargetPhraseCollection);
    vector<TargetPhrase*> targetPhrases
    = CreateTargetPhrases(sourcePhrase, outDir.path());
    vector<TargetPhrase*>::const_iterator iter;
    for (iter = targetPhrases.begin(); iter != targetPhrases.end(); ++iter) {
      TargetPhrase *tp = *iter;
      tpColl->Add(tp);
    }
    cache[hash] = CacheCollEntry(tpColl, clock());
    inputPath.SetTargetPhrases(*this, tpColl, NULL);
  }
}

std::vector<TargetPhrase*> PhraseDictionaryTransliteration::CreateTargetPhrases(const Phrase &sourcePhrase, const string &outDir) const
{
  std::vector<TargetPhrase*> ret;

  string outPath = outDir + "/out.txt";
  ifstream outStream(outPath.c_str());

  string line;
  while (getline(outStream, line)) {
    vector<string> toks;
    Tokenize(toks, line, "\t");
    UTIL_THROW_IF2(toks.size() != 2, "Error in transliteration output file. Expecting word\tscore");

    TargetPhrase *tp = new TargetPhrase(this);
    Word &word = tp->AddWord();
    word.CreateFromString(Output, m_output, toks[0], false);

    float score = Scan<float>(toks[1]);
    tp->GetScoreBreakdown().PlusEquals(this, score);

    // score of all other ff when this rule is being loaded
    tp->EvaluateInIsolation(sourcePhrase, GetFeaturesToApply());

    ret.push_back(tp);
  }

  outStream.close();

  return ret;
}

ChartRuleLookupManager* PhraseDictionaryTransliteration::CreateRuleLookupManager(const ChartParser &parser,
    const ChartCellCollectionBase &cellCollection,
    std::size_t /*maxChartSpan*/)
{
  return NULL;
  //return new ChartRuleLookupManagerSkeleton(parser, cellCollection, *this);
}

void
PhraseDictionaryTransliteration::
SetParameter(const std::string& key, const std::string& value)
{
  if (key == "moses-dir") {
    m_mosesDir = value;
  } else if (key == "script-dir") {
    m_scriptDir = value;
  } else if (key == "external-dir") {
    m_externalDir = value;
  } else if (key == "input-lang") {
    m_inputLang = value;
  } else if (key == "output-lang") {
    m_outputLang = value;
  } else {
    PhraseDictionary::SetParameter(key, value);
  }
}

TO_STRING_BODY(PhraseDictionaryTransliteration);

// friend
ostream& operator<<(ostream& out, const PhraseDictionaryTransliteration& phraseDict)
{
  return out;
}

}
