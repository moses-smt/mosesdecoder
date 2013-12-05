// vim:tabstop=2
#include <stdlib.h>
#include "TransliterationPhraseDictionary.h"
#include "moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerSkeleton.h"
#include "moses/DecodeGraph.h"
#include "moses/DecodeStep.h"

using namespace std;

namespace Moses
{
TransliterationPhraseDictionary::TransliterationPhraseDictionary(const std::string &line)
  : PhraseDictionary(line)
{
  ReadParameters();
  UTIL_THROW_IF2(m_mosesDir.empty() ||
		  m_scriptDir.empty() ||
		  m_externalDir.empty() ||
		  m_inputLang.empty() ||
		  m_outputLang.empty(), "Must specify all arguments");
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

    if (!SatisfyBackoff(inputPath)) {
    	continue;
    }

    const Phrase &sourcePhrase = inputPath.GetPhrase();

    if (sourcePhrase.GetSize() != 1) {
    	// only translit single words. A limitation of the translit script
    	continue;
    }

	// TRANSLITERATE
	char *ptr = tmpnam(NULL);
	string inFile(ptr);
	ptr = tmpnam(NULL);
	string outDir(ptr);

	ofstream inStream(inFile.c_str());
	inStream << sourcePhrase.ToString() << endl;
	inStream.close();

	string cmd = m_scriptDir + "/Transliteration/prepare-transliteration-phrase-table.pl" +
			" --transliteration-model-dir " + m_filePath +
			" --moses-src-dir " + m_mosesDir +
			" --external-bin-dir " + m_externalDir +
			" --input-extension " + m_inputLang +
			" --output-extension " + m_outputLang +
			" --oov-file " + inFile +
			" --out-dir " + outDir;

	int ret = system(cmd.c_str());
	UTIL_THROW_IF2(ret != 0, "Transliteration script error");

	TargetPhraseCollection *tpColl = new TargetPhraseCollection();
	vector<TargetPhrase*> targetPhrases = CreateTargetPhrases(sourcePhrase, outDir);
	vector<TargetPhrase*>::const_iterator iter;
	for (iter = targetPhrases.begin(); iter != targetPhrases.end(); ++iter) {
		TargetPhrase *tp = *iter;
		tpColl->Add(tp);
	}

	m_allTPColl.push_back(tpColl);
	inputPath.SetTargetPhrases(*this, tpColl, NULL);

	// clean up temporary files
	remove(inFile.c_str());

	cmd = "rm -rf " + outDir;
	system(cmd.c_str());
  }
}

std::vector<TargetPhrase*> TransliterationPhraseDictionary::CreateTargetPhrases(const Phrase &sourcePhrase, const string &outDir) const
{
	std::vector<TargetPhrase*> ret;

	string outPath = outDir + "/out.txt";
	ifstream outStream(outPath.c_str());

	string line;
	while (getline(outStream, line)) {
		vector<string> toks;
		Tokenize(toks, line, "\t");
		UTIL_THROW_IF2(toks.size() != 2, "Error in transliteration output file. Expecting word\tscore");

	  TargetPhrase *tp = new TargetPhrase();
	  Word &word = tp->AddWord();
	  word.CreateFromString(Output, m_output, toks[0], false);

	  float score = Scan<float>(toks[1]);
	  tp->GetScoreBreakdown().PlusEquals(this, score);

	  // score of all other ff when this rule is being loaded
	  tp->Evaluate(sourcePhrase, GetFeaturesToApply());

	  ret.push_back(tp);
	}

	outStream.close();

  return ret;
}

ChartRuleLookupManager* TransliterationPhraseDictionary::CreateRuleLookupManager(const ChartParser &parser,
    const ChartCellCollectionBase &cellCollection)
{
	return NULL;
  //return new ChartRuleLookupManagerSkeleton(parser, cellCollection, *this);
}

void
TransliterationPhraseDictionary::
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

bool TransliterationPhraseDictionary::SatisfyBackoff(const InputPath &inputPath) const
{
  const Phrase &sourcePhrase = inputPath.GetPhrase();

  assert(m_container);
  const DecodeGraph *decodeGraph = m_container->GetContainer();
  size_t backoff = decodeGraph->GetBackoff();

  if (backoff == 0) {
	  // ie. don't backoff. Collect ALL translations
	  return true;
  }

  if (sourcePhrase.GetSize() > backoff) {
	  // source phrase too big
	  return false;
  }

  // lookup translation only if no other translations
  InputPath::TargetPhrases::const_iterator iter;
  for (iter = inputPath.GetTargetPhrases().begin(); iter != inputPath.GetTargetPhrases().end(); ++iter) {
  	const std::pair<const TargetPhraseCollection*, const void*> &temp = iter->second;
  	const TargetPhraseCollection *tpCollPrev = temp.first;

  	if (tpCollPrev && tpCollPrev->GetSize()) {
  		// already have translation from another pt. Don't create translations
  		return false;
  	}
  }

  return true;
}

TO_STRING_BODY(TransliterationPhraseDictionary);

// friend
ostream& operator<<(ostream& out, const TransliterationPhraseDictionary& phraseDict)
{
  return out;
}

}
