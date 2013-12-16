
#include <boost/functional/hash.hpp>
#include "moses/FF/FFState.h"
#include "DALMWrapper.h"
#include "logger.h"
#include "dalm.h"
#include "vocabulary.h"
#include "moses/FactorTypeSet.h"
#include "moses/FactorCollection.h"
#include "moses/InputFileStream.h"
#include "util/exception.hh"
#include "ChartState.h"

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
	DALM::State *state;

public:
	DALMState(unsigned short order){
		state = new DALM::State(order);
	}

	DALMState(const DALMState &from){
		state = new DALM::State(*from.state);
	}

  virtual ~DALMState(){
		delete state;
	}

  virtual int Compare(const FFState& other) const{
		const DALMState &o = static_cast<const DALMState &>(other);
		if(state->get_count() < o.state->get_count()) return -1;
		else if(state->get_count() > o.state->get_count()) return 1;
		else{
			// s->count==o.state->count;
			unsigned short count = state->get_count();
			for(size_t i = 0; i < count; i++){
				DALM::VocabId w1 = state->get_word(i);
				DALM::VocabId w2 = o.state->get_word(i);
				if(w1 < w2){
					return -1;
				}else if(w1 > w2){
					return 1;
				}
			}
			return 0;
		}
	}

	DALM::State *get_state() const{
		return state;
	}
	
	void refresh(){
		state->refresh();
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
	string model; // Path to the double-array file.
	string words; // Path to the vocabulary file.
	string wordstxt; //Path to the vocabulary file in text format.
	read_ini(m_filePath.c_str(), model, words, wordstxt);

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
	m_lm = new DALM::LM(model, *m_vocab, *m_logger);
	
	wid_start = m_vocab->lookup(BOS_);
	wid_end = m_vocab->lookup(EOS_);

	// vocab mapping
	CreateVocabMapping(wordstxt);
	
	FactorCollection &collection = FactorCollection::Instance();
	m_beginSentenceFactor = collection.AddFactor(BOS_);
}

const FFState *LanguageModelDALM::EmptyHypothesisState(const InputType &/*input*/) const{
	DALMState *s = new DALMState(m_nGramOrder);
	m_lm->init_state(*s->get_state());
	return s;
}

void LanguageModelDALM::CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, size_t &oovCount) const{
  fullScore  = 0;
  ngramScore = 0;

  oovCount = 0;

  size_t phraseSize = phrase.GetSize();
  if (!phraseSize) return;
  
  DALMState *dalm_state = new DALMState(m_nGramOrder);
  
  size_t currPos = 0;
  size_t hist_count = 0;
  
  while (currPos < phraseSize) {
    const Word &word = phrase.GetWord(currPos);
    hist_count++;

    if (word.IsNonTerminal()) {
      // do nothing. reset ngram. needed to score target phrases during pt loading in chart decoding
      dalm_state->refresh();
      hist_count = 0;
    } else {
      if (word.GetFactor(m_factorType) == m_beginSentenceFactor) {
        // do nothing, don't include prob for <s> unigram
        if (currPos != 0) {
          std::cerr << "Either your data contains <s> in a position other than the first word or your language model is missing <s>.  Did you build your ARPA using IRSTLM and forget to run add-start-end.sh?" << std::endl;
          abort();
        }
    		m_lm->init_state(*dalm_state->get_state());
      } else {
        LMResult result = GetValue(word, dalm_state->get_state());
        fullScore += result.score;
        if (hist_count >= m_nGramOrder) ngramScore += result.score;
        if (result.unknown) ++oovCount;
      }
    }

    currPos++;
  }
	delete dalm_state;
}

LMResult LanguageModelDALM::GetValue(DALM::VocabId wid, DALM::State* finalState) const{
  LMResult ret;

  // last word is unk?
  ret.unknown = (wid == m_vocab->unk());

  // calc score.
  float score = m_lm->query(wid, *finalState);
  score = TransformLMScore(score);
  ret.score = score;

  return ret;
}

LMResult LanguageModelDALM::GetValue(const Word &word, DALM::State* finalState) const
{
  DALM::VocabId wid = GetVocabId(word.GetFactor(m_factorType));
  
  return GetValue(wid, finalState);
}

FFState *LanguageModelDALM::Evaluate(const Hypothesis &hypo, const FFState *ps, ScoreComponentCollection *out) const{
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
  
  std::size_t position = begin;
  float score = 0.0;
  for(; position < adjust_end; position++){
  	score += GetValue(hypo.GetWord(position), dalm_state->get_state()).score;
  }
  
  if (hypo.IsSourceCompleted()) {
    // Score end of sentence.
    std::vector<DALM::VocabId> indices(m_nGramOrder-1);
    const DALM::VocabId *last = LastIDs(hypo, &indices.front());
    m_lm->set_state(&indices.front(), (last-&indices.front()), *dalm_state->get_state());
    
    float s = GetValue(wid_end, dalm_state->get_state()).score;
    score += s;
  } else if (adjust_end < end) {
    // Get state after adding a long phrase.
    std::vector<DALM::VocabId> indices(m_nGramOrder-1);
    const DALM::VocabId *last = LastIDs(hypo, &indices.front());
    m_lm->set_state(&indices.front(), (last-&indices.front()), *dalm_state->get_state());
  }

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

FFState *LanguageModelDALM::EvaluateChart(const ChartHypothesis& hypo, int featureID, ScoreComponentCollection *out) const{
  LanguageModelChartState *ret = new LanguageModelChartState(hypo, featureID, m_nGramOrder);
  // initialize language model context state
	DALMState *dalm_state = new DALMState(m_nGramOrder);

  // initial language model scores
  float prefixScore = 0.0;    // not yet final for initial words (lack context)
  float finalizedScore = 0.0; // finalized, has sufficient context

  // get index map for underlying hypotheses
  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
    hypo.GetCurrTargetPhrase().GetAlignNonTerm().GetNonTermIndexMap();

  // loop over rule
  for (size_t phrasePos = 0, wordPos = 0;
       phrasePos < hypo.GetCurrTargetPhrase().GetSize();
       phrasePos++) {
    // consult rule for either word or non-terminal
    const Word &word = hypo.GetCurrTargetPhrase().GetWord(phrasePos);

    // regular word
    if (!word.IsNonTerminal()) {
      // beginning of sentence symbol <s>? -> just update state
      if (word.GetFactor(m_factorType) == m_beginSentenceFactor) {
      UTIL_THROW_IF2(phrasePos != 0,
          "Sentence start symbol must be at the beginning of sentence");
				m_lm->init_state(*dalm_state->get_state());
      }
      // score a regular word added by the rule
      else {
        updateChartScore( &prefixScore, &finalizedScore, GetValue(word, dalm_state->get_state()).score, ++wordPos );
      }
    }

    // non-terminal, add phrase from underlying hypothesis
    else {
      // look up underlying hypothesis
      size_t nonTermIndex = nonTermIndexMap[phrasePos];
      const ChartHypothesis *prevHypo = hypo.GetPrevHypo(nonTermIndex);

      const LanguageModelChartState* prevState =
        static_cast<const LanguageModelChartState*>(prevHypo->GetFFState(featureID));
      
      size_t subPhraseLength = prevState->GetNumTargetTerminals();
      // special case: rule starts with non-terminal -> copy everything
      if (phrasePos == 0) {

        // get prefixScore and finalizedScore
        prefixScore = prevState->GetPrefixScore();
        finalizedScore = prevHypo->GetScoreBreakdown().GetScoresForProducer(this)[0] - prefixScore;

        // get language model state
        delete dalm_state;
        dalm_state = new DALMState( *static_cast<DALMState*>(prevState->GetRightContext()) );
				wordPos += subPhraseLength;
      }

      // internal non-terminal
      else {
        // score its prefix
				size_t wpos = wordPos;
        for(size_t prefixPos = 0;
            prefixPos < m_nGramOrder-1 // up to LM order window
            && prefixPos < subPhraseLength; // up to length
            prefixPos++) {
          const Word &word = prevState->GetPrefix().GetWord(prefixPos);
          updateChartScore( &prefixScore, &finalizedScore, GetValue(word, dalm_state->get_state()).score, ++wpos );
        }
				wordPos += subPhraseLength;

        // check if we are dealing with a large sub-phrase
        if (subPhraseLength > m_nGramOrder - 1) {
          // add its finalized language model score
          finalizedScore +=
            prevHypo->GetScoreBreakdown().GetScoresForProducer(this)[0] // full score
            - prevState->GetPrefixScore();                              // - prefix score

          // copy language model state
          delete dalm_state;
          dalm_state = new DALMState( *static_cast<DALMState*>(prevState->GetRightContext()) );
        }
      }
    }
  }

  // assign combined score to score breakdown
  out->Assign(this, prefixScore + finalizedScore);

  ret->Set(prefixScore, dalm_state);
  return ret;
}

bool LanguageModelDALM::IsUseable(const FactorMask &mask) const
{
  bool ret = mask[m_factorType];
  return ret;
}

void LanguageModelDALM::CreateVocabMapping(const std::string &wordstxt)
{
  InputFileStream vocabStrm(wordstxt);

  string line;
  while(getline(vocabStrm, line)) {
	  const Factor *factor = FactorCollection::Instance().AddFactor(line);
	  DALM::VocabId wid = m_vocab->lookup(line.c_str());

	  VocabMap::value_type entry(factor, wid);
	  m_vocabMap.insert(entry);
  }

}

DALM::VocabId LanguageModelDALM::GetVocabId(const Factor *factor) const
{
	VocabMap::left_map::const_iterator iter;
	iter = m_vocabMap.left.find(factor);
	if (iter != m_vocabMap.left.end()) {
		return iter->second;
	}
	else {
		// not in mapping. Must be UNK
		return m_vocab->unk();
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
    LanguageModel::SetParameter(key, value);
  }
}

void LanguageModelDALM::updateChartScore(float *prefixScore, float *finalizedScore, float score, size_t wordPos) const
{
  if (wordPos < m_nGramOrder) {
    *prefixScore += score;
  } else {
    *finalizedScore += score;
  }
}

}
