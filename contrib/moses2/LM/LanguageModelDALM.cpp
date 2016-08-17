/*
 * LanguageModelDALM.cpp
 *
 *  Created on: 5 Dec 2015
 *      Author: hieu
 */

#include "LanguageModelDALM.h"
#include "../TypeDef.h"
#include "../System.h"
#include "dalm.h"
#include "util/exception.hh"
#include "../legacy/InputFileStream.h"

using namespace std;

namespace Moses2
{

//////////////////////////////////////////////////////////////////////////////////////////
class Murmur: public DALM::State::HashFunction
{
public:
  Murmur(std::size_t seed=0): seed(seed) {
  }
  virtual std::size_t operator()(const DALM::VocabId *words, std::size_t size) const {
    return util::MurmurHashNative(words, sizeof(DALM::VocabId) * size, seed);
  }
private:
  std::size_t seed;
};

//////////////////////////////////////////////////////////////////////////////////////////
class DALMState : public FFState
{
private:
  DALM::State state;

public:
  DALMState() {
  }

  DALMState(const DALMState &from) {
    state = from.state;
  }

  virtual ~DALMState() {
  }

  void reset(const DALMState &from) {
    state = from.state;
  }

  virtual int Compare(const FFState& other) const {
    const DALMState &o = static_cast<const DALMState &>(other);
    if(state.get_count() < o.state.get_count()) return -1;
    else if(state.get_count() > o.state.get_count()) return 1;
    else return state.compare(o.state);
  }

  virtual size_t hash() const {
    // imitate KenLM
    return state.hash(Murmur());
  }

  virtual bool operator==(const FFState& other) const {
    const DALMState &o = static_cast<const DALMState &>(other);
    return state.compare(o.state) == 0;
  }

  DALM::State &get_state() {
    return state;
  }

  void refresh() {
    state.refresh();
  }

  virtual std::string ToString() const
  { return "DALM state"; }

};

//////////////////////////////////////////////////////////////////////////////////////////////////////
inline void read_ini(const char *inifile, string &model, string &words, string &wordstxt)
{
  ifstream ifs(inifile);
  string line;

  getline(ifs, line);
  while(ifs) {
	unsigned int pos = line.find("=");
	string key = line.substr(0, pos);
	string value = line.substr(pos+1, line.size()-pos);
	if(key=="MODEL") {
	  model = value;
	} else if(key=="WORDS") {
	  words = value;
	} else if(key=="WORDSTXT") {
	  wordstxt = value;
	}
	getline(ifs, line);
  }
}
/////////////////////////

LanguageModelDALM::LanguageModelDALM(size_t startInd, const std::string &line)
:StatefulFeatureFunction(startInd, line)
{
	ReadParameters();
}

LanguageModelDALM::~LanguageModelDALM() {
	// TODO Auto-generated destructor stub
}

void LanguageModelDALM::Load(System &system)
{
  /////////////////////
  // READING INIFILE //
  /////////////////////
  string inifile= m_filePath + "/dalm.ini";

  string model; // Path to the double-array file.
  string words; // Path to the vocabulary file.
  string wordstxt; //Path to the vocabulary file in text format.
  read_ini(inifile.c_str(), model, words, wordstxt);

  model = m_filePath + "/" + model;
  words = m_filePath + "/" + words;
  wordstxt = m_filePath + "/" + wordstxt;

  UTIL_THROW_IF(model.empty() || words.empty() || wordstxt.empty(),
                util::FileOpenException,
                "Failed to read DALM ini file " << m_filePath << ". Probably doesn't exist");

  ////////////////
  // LOADING LM //
  ////////////////

  // Preparing a logger object.
  m_logger = new DALM::Logger(stderr);
  m_logger->setLevel(DALM::LOGGER_INFO);

  // Load the vocabulary file.
  m_vocab = new DALM::Vocabulary(words, *m_logger);

  // Load the language model.
  m_lm = new DALM::LM(model, *m_vocab, m_nGramOrder, *m_logger);

  wid_start = m_vocab->lookup(BOS_);
  wid_end = m_vocab->lookup(EOS_);

  // vocab mapping
  CreateVocabMapping(wordstxt, system);

  m_beginSentenceFactor = system.GetVocab().AddFactor(BOS_, system);
}

void LanguageModelDALM::CreateVocabMapping(const std::string &wordstxt, const System &system)
{
  InputFileStream vocabStrm(wordstxt);

  std::vector< std::pair<std::size_t, DALM::VocabId> > vlist;
  string line;
  std::size_t max_fid = 0;
  while(getline(vocabStrm, line)) {
	const Factor *factor = system.GetVocab().AddFactor(line, system);
	std::size_t fid = factor->GetId();
	DALM::VocabId wid = m_vocab->lookup(line.c_str());

	vlist.push_back(std::pair<std::size_t, DALM::VocabId>(fid, wid));
	if(max_fid < fid) max_fid = fid;
  }

  for(std::size_t i = 0; i < m_vocabMap.size(); i++) {
	m_vocabMap[i] = m_vocab->unk();
  }

  m_vocabMap.resize(max_fid+1, m_vocab->unk());
  std::vector< std::pair<std::size_t, DALM::VocabId> >::iterator it = vlist.begin();
  while(it != vlist.end()) {
	std::pair<std::size_t, DALM::VocabId> &entry = *it;
	m_vocabMap[entry.first] = entry.second;

	++it;
  }
}

void LanguageModelDALM::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "factor") {
	m_factorType = Scan<FactorType>(value);
  } else if (key == "order") {
	m_nGramOrder = Scan<size_t>(value);
  } else if (key == "path") {
	m_filePath = value;
  } else {
	  StatefulFeatureFunction::SetParameter(key, value);
  }
  m_ContextSize = m_nGramOrder-1;
}

FFState* LanguageModelDALM::BlankState(MemPool &pool, const System &sys) const
{
	DALMState *state = new DALMState();
	return state;
}

void LanguageModelDALM::EmptyHypothesisState(FFState &state,
		const ManagerBase &mgr,
		const InputType &input,
		const Hypothesis &hypo) const
{
  DALMState &dalmState = static_cast<DALMState&>(state);
  m_lm->init_state(dalmState.get_state());
}

 void LanguageModelDALM::EvaluateInIsolation(MemPool &pool,
		 const System &system,
		 const Phrase &source,
		 const TargetPhraseImpl &targetPhrase,
         Scores &scores,
		 SCORE &estimatedScore) const
 {

 }

void LanguageModelDALM::EvaluateWhenApplied(const ManagerBase &mgr,
const Hypothesis &hypo,
const FFState &prevState,
Scores &scores,
FFState &state) const
{

}

void LanguageModelDALM::EvaluateWhenApplied(const SCFG::Manager &mgr,
    const SCFG::Hypothesis &hypo, int featureID, Scores &scores,
    FFState &state) const
{
  UTIL_THROW2("Not implemented");
}

}

