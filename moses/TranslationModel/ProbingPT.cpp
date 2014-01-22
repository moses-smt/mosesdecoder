// vim:tabstop=2
#include "ProbingPT.h"
#include "moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerSkeleton.h"
#include "quering.hh"

using namespace std;

namespace Moses
{
ProbingPT::ProbingPT(const std::string &line)
: PhraseDictionary(line)
,m_engine(NULL)
{
  ReadParameters();
}

ProbingPT::~ProbingPT()
{
  delete m_engine;
}

void ProbingPT::Load()
{
	SetFeaturesToApply();

	string hashPath = m_filePath + "/probing_hash.dat";
	string binPath = m_filePath + "/binfile.dat";
	string vocabPath = m_filePath + "/vocabid.dat";
	string tableSize = "";
	string maxEntry = "";

	m_engine = new QueryEngine(hashPath.c_str(), binPath.c_str(), vocabPath.c_str(), tableSize.c_str(), maxEntry.c_str());

}

void ProbingPT::InitializeForInput(InputType const& source)
{
  ReduceCache();
}

void ProbingPT::GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const
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

TargetPhrase *ProbingPT::CreateTargetPhrase(const Phrase &sourcePhrase) const
{
  // create a target phrase from the 1st word of the source, prefix with 'ProbingPT:'
  assert(sourcePhrase.GetSize());

  vector<uint64_t> source;
  std::pair<bool, std::vector<target_text>> query_result;

  //Actual lookup
  std::string cinstr = "adsadsadas fasdasd sad sadasd";
  query_result = queries.query(source);

	if (query_result.first) {
		queries.printTargetInfo(query_result.second);
	} else {
		std::cout << "Key not found!" << std::endl;
	}

  string str = sourcePhrase.GetWord(0).GetFactor(0)->GetString().as_string();
  str = "ProbingPT:" + str;

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

ChartRuleLookupManager* ProbingPT::CreateRuleLookupManager(const ChartParser &parser,
    const ChartCellCollectionBase &cellCollection)
{
  abort();
  return NULL;
}

TO_STRING_BODY(ProbingPT);

// friend
ostream& operator<<(ostream& out, const ProbingPT& phraseDict)
{
  return out;
}

}
