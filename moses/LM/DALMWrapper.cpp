
//#include <boost/functional/hash.hpp>
#include <algorithm>
#include "moses/FF/FFState.h"
#include "DALMWrapper.h"
#include "logger.h"
#include "dalm.h"
#include "vocabulary.h"
#include "moses/FactorTypeSet.h"
#include "moses/FactorCollection.h"
#include "moses/InputFileStream.h"
#include "util/exception.hh"
#include "moses/ChartHypothesis.h"
#include "moses/ChartManager.h"

using namespace std;

/////////////////////////
void read_ini(const char *inifile, string &model, string &words, string &wordstxt){
	ifstream ifs(inifile);
	string line;

	getline(ifs, line);
	while(ifs){
		unsigned int pos = line.find("=");
		string key = line.substr(0, pos);
		string value = line.substr(pos+1, line.size()-pos);
		if(key=="MODEL"){
			model = value;
		}else if(key=="WORDS"){
			words = value;
		}else if(key=="WORDSTXT"){
			wordstxt = value;
		}
		getline(ifs, line);
	}
}
/////////////////////////

namespace Moses
{

class DALMState : public FFState
{
private:
	DALM::State state;

public:
	DALMState(){
	}

	DALMState(const DALMState &from){
		state = from.state;
	}

  virtual ~DALMState(){
	}

	void reset(const DALMState &from){
		state = from.state;
	}

  virtual int Compare(const FFState& other) const{
		const DALMState &o = static_cast<const DALMState &>(other);
		if(state.get_count() < o.state.get_count()) return -1;
		else if(state.get_count() > o.state.get_count()) return 1;
		else return state.compare(o.state);
	}

	DALM::State &get_state(){
		return state;
	}
	
	void refresh(){
		state.refresh();
	}
};

class DALMChartState : public FFState
{
private:
	DALM::Fragment prefixFragments[DALM_MAX_ORDER-1];
	unsigned char prefixLength;
	DALM::State rightContext;
	bool isLarge;
	size_t hypoSize;

public:
	DALMChartState()
		: prefixLength(0),
		  isLarge(false)
	{}

	/*
	DALMChartState(const DALMChartState &other)
		: prefixLength(other.prefixLength),
			rightContext(other.rightContext),
			isLarge(other.isLarge)
	{
		std::copy(
			other.prefixFragments, 
			other.prefixFragments+other.prefixLength,
			prefixFragments
			);
	}
	*/

	virtual ~DALMChartState(){
	}

	/*
	DALMChartState &operator=(const DALMChartState &other){
		prefixLength = other.prefixLength;
		std::copy(
			other.prefixFragments, 
			other.prefixFragments+other.prefixLength,
			prefixFragments
			);
		rightContext = other.rightContext;
		isLarge=other.isLarge;

		return *this;
	}
	*/

	inline unsigned char GetPrefixLength() const{
		return prefixLength;
	}

	inline unsigned char &GetPrefixLength(){
		return prefixLength;
	}

	inline const DALM::Fragment *GetPrefixFragments() const{
		return prefixFragments;
	}

	inline DALM::Fragment *GetPrefixFragments(){
		return prefixFragments;
	}

	inline const DALM::State &GetRightContext() const{
		return rightContext;
	}

	inline DALM::State &GetRightContext() {
		return rightContext;
	}

	inline bool LargeEnough() const{
		return isLarge;
	}

	inline void SetAsLarge() {
		isLarge=true;
	}
	
	inline size_t &GetHypoSize() {
		return hypoSize;
	}
	inline size_t GetHypoSize() const {
		return hypoSize;
	}

  virtual int Compare(const FFState& other) const{
		const DALMChartState &o = static_cast<const DALMChartState &>(other);
		if(prefixLength < o.prefixLength) return -1;
		if(prefixLength > o.prefixLength) return 1;
		if(prefixLength!=0){
			const DALM::Fragment &f = prefixFragments[prefixLength-1];
			const DALM::Fragment &of = o.prefixFragments[prefixLength-1];
			int ret = DALM::compare_fragments(f,of);
			if(ret != 0) return ret;
		}
		if(isLarge != o.isLarge) return (int)isLarge - (int)o.isLarge;
		if(rightContext.get_count() < o.rightContext.get_count()) return -1;
		if(rightContext.get_count() > o.rightContext.get_count()) return 1;
		return rightContext.compare(o.rightContext);
	}
};

LanguageModelDALM::LanguageModelDALM(const std::string &line)
  :LanguageModel(line)
{
  ReadParameters();

  if (m_factorType == NOT_FOUND) {
    m_factorType = 0;
  }
}

LanguageModelDALM::~LanguageModelDALM()
{
	delete m_logger;
	delete m_vocab;
	delete m_lm;
}

void LanguageModelDALM::Load()
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
	CreateVocabMapping(wordstxt);
	
	FactorCollection &collection = FactorCollection::Instance();
	m_beginSentenceFactor = collection.AddFactor(BOS_);
}

const FFState *LanguageModelDALM::EmptyHypothesisState(const InputType &/*input*/) const{
	DALMState *s = new DALMState();
	m_lm->init_state(s->get_state());
	return s;
}

void LanguageModelDALM::CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, size_t &oovCount) const{
  fullScore  = 0;
  ngramScore = 0;

  oovCount = 0;

  size_t phraseSize = phrase.GetSize();
  if (!phraseSize) return;
  
  size_t currPos = 0;
  size_t hist_count = 0;
	DALM::State state;

	if(phrase.GetWord(0).GetFactor(m_factorType) == m_beginSentenceFactor){
		m_lm->init_state(state);
		currPos++;
		hist_count++;
	}
  
	float score;
  while (currPos < phraseSize) {
    const Word &word = phrase.GetWord(currPos);
    hist_count++;

    if (word.IsNonTerminal()) {
      state.refresh();
      hist_count = 0;
    } else {
			DALM::VocabId wid = GetVocabId(word.GetFactor(m_factorType));
			score = m_lm->query(wid, state);
  		fullScore += score;
      if (hist_count >= m_nGramOrder) ngramScore += score;
      if (wid==m_vocab->unk()) ++oovCount;
    }

    currPos++;
  }

	fullScore = TransformLMScore(fullScore);
	ngramScore = TransformLMScore(ngramScore);
}

FFState *LanguageModelDALM::EvaluateWhenApplied(const Hypothesis &hypo, const FFState *ps, ScoreComponentCollection *out) const{
  // In this function, we only compute the LM scores of n-grams that overlap a
  // phrase boundary. Phrase-internal scores are taken directly from the
  // translation option.

	const DALMState *dalm_ps = static_cast<const DALMState *>(ps);
	
  // Empty phrase added? nothing to be done
  if (hypo.GetCurrTargetLength() == 0){
    return dalm_ps ? new DALMState(*dalm_ps) : NULL;
  }
  
  const std::size_t begin = hypo.GetCurrTargetWordsRange().GetStartPos();
  //[begin, end) in STL-like fashion.
  const std::size_t end = hypo.GetCurrTargetWordsRange().GetEndPos() + 1;
  const std::size_t adjust_end = std::min(end, begin + m_nGramOrder - 1);
  
  DALMState *dalm_state = new DALMState(*dalm_ps);
	DALM::State &state = dalm_state->get_state();
  float score = 0.0;
  for(std::size_t position=begin; position < adjust_end; position++){
  	score += m_lm->query(GetVocabId(hypo.GetWord(position).GetFactor(m_factorType)), state);
  }
  
  if (hypo.IsSourceCompleted()) {
    // Score end of sentence.
    std::vector<DALM::VocabId> indices(m_nGramOrder-1);
    const DALM::VocabId *last = LastIDs(hypo, &indices.front());
    m_lm->set_state(&indices.front(), (last-&indices.front()), state);
    
  	score += m_lm->query(wid_end, state);
  } else if (adjust_end < end) {
    // Get state after adding a long phrase.
    std::vector<DALM::VocabId> indices(m_nGramOrder-1);
    const DALM::VocabId *last = LastIDs(hypo, &indices.front());
    m_lm->set_state(&indices.front(), (last-&indices.front()), state);
  }

	score = TransformLMScore(score);
  if (OOVFeatureEnabled()) {
    std::vector<float> scores(2);
    scores[0] = score;
    scores[1] = 0.0;
    out->PlusEquals(this, scores);
  } else {
    out->PlusEquals(this, score);
  }
	
  return dalm_state;
}

FFState *LanguageModelDALM::EvaluateWhenApplied(const ChartHypothesis& hypo, int featureID, ScoreComponentCollection *out) const{
  // initialize language model context state
 	DALMChartState *newState = new DALMChartState();
	DALM::State &state = newState->GetRightContext();

	DALM::Fragment *prefixFragments = newState->GetPrefixFragments();
	unsigned char &prefixLength = newState->GetPrefixLength();
	size_t &hypoSizeAll = newState->GetHypoSize();

  // initial language model scores
  float hypoScore = 0.0;      // diffs of scores.

	const TargetPhrase &targetPhrase = hypo.GetCurrTargetPhrase();
	size_t hypoSize = targetPhrase.GetSize();
	hypoSizeAll = hypoSize;

  // get index map for underlying hypotheses
  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
    targetPhrase.GetAlignNonTerm().GetNonTermIndexMap();

	size_t phrasePos = 0;

	// begginig of sentence.
	if(hypoSize > 0){
		const Word &word = targetPhrase.GetWord(0);
		if(word.GetFactor(m_factorType) == m_beginSentenceFactor){
			m_lm->init_state(state);
			// state is finalized.
			newState->SetAsLarge();
			phrasePos++;
		}else if(word.IsNonTerminal()){
      // special case: rule starts with non-terminal -> copy everything

      const ChartHypothesis *prevHypo = hypo.GetPrevHypo(nonTermIndexMap[0]);
      const DALMChartState* prevState =
        static_cast<const DALMChartState*>(prevHypo->GetFFState(featureID));

			// copy chart state
			(*newState) = (*prevState);
			hypoSizeAll = hypoSize+prevState->GetHypoSize()-1;

			phrasePos++;
		}
  }

  // loop over rule
  for (; phrasePos < hypoSize; phrasePos++) {

    // consult rule for either word or non-terminal
    const Word &word = targetPhrase.GetWord(phrasePos);

    // regular word
    if (!word.IsNonTerminal()) {
			EvaluateTerminal(
					word, hypoScore, 
					newState, state, 
					prefixFragments, prefixLength
					);
    }

    // non-terminal, add phrase from underlying hypothesis
    // internal non-terminal
    else {
      // look up underlying hypothesis
  		const ChartHypothesis *prevHypo = hypo.GetPrevHypo(nonTermIndexMap[phrasePos]);
  		const DALMChartState* prevState =
    		static_cast<const DALMChartState*>(prevHypo->GetFFState(featureID));
			size_t prevTargetPhraseLength = prevHypo->GetCurrTargetPhrase().GetSize();
			hypoSizeAll += prevState->GetHypoSize()-1;

			EvaluateNonTerminal(
				word, hypoScore, 
				newState, state, 
				prefixFragments, prefixLength, 
				prevState, prevTargetPhraseLength
				);
    }
  }

  // assign combined score to score breakdown
  out->PlusEquals(this, TransformLMScore(hypoScore));

  return newState;
}

bool LanguageModelDALM::IsUseable(const FactorMask &mask) const
{
  return mask[m_factorType];
}

void LanguageModelDALM::CreateVocabMapping(const std::string &wordstxt)
{
  InputFileStream vocabStrm(wordstxt);

	std::vector< std::pair<std::size_t, DALM::VocabId> > vlist;
  string line;
	std::size_t max_fid = 0;
  while(getline(vocabStrm, line)) {
	  const Factor *factor = FactorCollection::Instance().AddFactor(line);
		std::size_t fid = factor->GetId();
	  DALM::VocabId wid = m_vocab->lookup(line.c_str());

	  vlist.push_back(std::pair<std::size_t, DALM::VocabId>(fid, wid));
		if(max_fid < fid) max_fid = fid;
  }

	for(std::size_t i = 0; i < m_vocabMap.size(); i++){
		m_vocabMap[i] = m_vocab->unk();
	}

	m_vocabMap.resize(max_fid+1, m_vocab->unk());
	std::vector< std::pair<std::size_t, DALM::VocabId> >::iterator it = vlist.begin();
	while(it != vlist.end()){
		std::pair<std::size_t, DALM::VocabId> &entry = *it;
		m_vocabMap[entry.first] = entry.second;

		++it;
	}
}

DALM::VocabId LanguageModelDALM::GetVocabId(const Factor *factor) const
{
	std::size_t fid = factor->GetId();
	return (m_vocabMap.size() > fid)? m_vocabMap[fid] : m_vocab->unk();
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
    LanguageModel::SetParameter(key, value);
  }
	m_ContextSize = m_nGramOrder-1;
}

void LanguageModelDALM::EvaluateTerminal(
	const Word &word, 
	float &hypoScore,
	DALMChartState *newState, 
	DALM::State &state, 
	DALM::Fragment *prefixFragments,
	unsigned char &prefixLength) const{

	DALM::VocabId wid = GetVocabId(word.GetFactor(m_factorType));
	if (newState->LargeEnough()) {
		float score = m_lm->query(wid, state);
		hypoScore += score;
	}else{
		float score = m_lm->query(wid, state, prefixFragments[prefixLength]);

		if(score > 0){
			hypoScore -= score;
			newState->SetAsLarge();
		}else if(state.get_count()<=prefixLength){
			hypoScore += score;
			prefixLength++;
			newState->SetAsLarge();
		}else{
			hypoScore += score;
			prefixLength++;
			if(prefixLength >= m_ContextSize) newState->SetAsLarge();
		}
	}
}

void LanguageModelDALM::EvaluateNonTerminal(
  const Word &word,
  float &hypoScore,
  DALMChartState *newState,
  DALM::State &state,
  DALM::Fragment *prefixFragments,
  unsigned char &prefixLength,
	const DALMChartState *prevState,
	size_t prevTargetPhraseLength
	) const{

  const unsigned char prevPrefixLength = prevState->GetPrefixLength();
	const DALM::Fragment *prevPrefixFragments = prevState->GetPrefixFragments();

	if(prevPrefixLength == 0){
		newState->SetAsLarge();
		hypoScore += state.sum_bows(0, state.get_count());
		state = prevState->GetRightContext();
		return;
	}
	if(!state.has_context()){
		newState->SetAsLarge();
		state = prevState->GetRightContext();
		return;
	}
	DALM::Gap gap(state);

  // score its prefix
  for(size_t prefixPos = 0; prefixPos < prevPrefixLength; prefixPos++) {
		const DALM::Fragment &f = prevPrefixFragments[prefixPos];
		if (newState->LargeEnough()) {
			float score = m_lm->query(f, state, gap);
			hypoScore += score;

			if(!gap.is_extended()){
				state = prevState->GetRightContext();
				return;
			}else if(state.get_count() <= prefixPos+1){
				state = prevState->GetRightContext();
				return;
			}
		} else {
			DALM::Fragment &fnew = prefixFragments[prefixLength];
			float score = m_lm->query(f, state, gap, fnew);
			hypoScore += score;

			if(!gap.is_extended()){
				newState->SetAsLarge();
				state = prevState->GetRightContext();
				return;
			}else if(state.get_count() <= prefixPos+1){
				if(!gap.is_finalized()) prefixLength++;
				newState->SetAsLarge();
				state = prevState->GetRightContext();
				return;
			}else if(gap.is_finalized()){
				newState->SetAsLarge();
			}else{
				prefixLength++;
				if(prefixLength >= m_ContextSize) newState->SetAsLarge();
			}
		}
		gap.succ();
  }

  // check if we are dealing with a large sub-phrase
  if (prevState->LargeEnough()) {
    newState->SetAsLarge();
		if(prevPrefixLength < prevState->GetHypoSize()){
 			hypoScore += state.sum_bows(prevPrefixLength, state.get_count());
  	}
   	// copy language model state
		state = prevState->GetRightContext();
  } else {
    m_lm->set_state(state, prevState->GetRightContext(), prevPrefixFragments, gap);
  }
}

}
