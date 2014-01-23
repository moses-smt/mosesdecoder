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

	m_engine = new QueryEngine(m_filePath.c_str());

	// vocab
	const std::map<uint64_t, std::string> &probingVocab = m_engine->getVocab();
	std::map<uint64_t, std::string>::const_iterator iter;
	for (iter = probingVocab.begin(); iter != probingVocab.end(); ++iter) {

	}
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

    TargetPhraseCollection *tpColl = CreateTargetPhrase(sourcePhrase);

    // add target phrase to phrase-table cache
    size_t hash = hash_value(sourcePhrase);
	std::pair<const TargetPhraseCollection*, clock_t> value(tpColl, clock());
	cache[hash] = value;

    inputPath.SetTargetPhrases(*this, tpColl, NULL);
  }
}

TargetPhraseCollection *ProbingPT::CreateTargetPhrase(const Phrase &sourcePhrase) const
{
  // create a target phrase from the 1st word of the source, prefix with 'ProbingPT:'
  assert(sourcePhrase.GetSize());

  vector<uint64_t> source;
  std::pair<bool, std::vector<target_text> > query_result;

  TargetPhraseCollection *tpColl = NULL;

  //Actual lookup
  std::string cinstr = sourcePhrase.ToString();
  query_result = m_engine->query(source);

  if (query_result.first) {
	  m_engine->printTargetInfo(query_result.second);
	  tpColl = new TargetPhraseCollection();

	  const std::vector<target_text> &probingTargetPhrases = query_result.second;
	  for (size_t i = 0; i < probingTargetPhrases.size(); ++i) {
		  const target_text &probingTargetPhrase = probingTargetPhrases[i];
		  TargetPhrase *tp = CreateTargetPhrase(sourcePhrase, probingTargetPhrase);

		  tpColl->Add(tp);
	  }

  } else {
    std::cerr << "Key not found!" << std::endl;
  }

  return tpColl;
}

TargetPhrase *ProbingPT::CreateTargetPhrase(const Phrase &sourcePhrase, const target_text &probingTargetPhrase) const
{
  const std::vector<uint64_t> &probingPhrase = probingTargetPhrase.target_phrase;
  size_t size = probingPhrase.size();

  TargetPhrase *tp = new TargetPhrase();

  // words
  for (size_t i = 0; i < size; ++i) {
	  string str; // TODO get string from vocab id. Preferably create map<id, factor>

	  Word &word = tp->AddWord();
	  word.CreateFromString(Output, m_output, str, false);
  }

  // score for this phrase table
  vector<float> scores;

  // convert double --> float --> log. TODO - tell nik to store as float
  const vector<double> &prob = probingTargetPhrase.prob;
  std::copy(prob.begin(), prob.end(), back_inserter(scores));
  std::transform(scores.begin(), scores.end(), scores.begin(),TransformScore);

  tp->GetScoreBreakdown().PlusEquals(this, scores);

  // alignment
  const std::vector<int> &alignS = probingTargetPhrase.word_all1;
  const std::vector<int> &alignT = probingTargetPhrase.word_all2;
  assert(alignS.size() == alignT.size());

  /*
  AlignmentInfo &aligns = tp->GetAlignTerm();
  for (size_t i = 0; i < alignS.size(); ++i) {
	  aligns.Add(alignS[i], alignT[i]);
  }
  */

  // score of all other ff when this rule is being loaded
  tp->Evaluate(sourcePhrase, GetFeaturesToApply());
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
