
#include "DALM.h"
#include "moses/FactorCollection.h"
#include "logger.h"
//#include "DALM/include/lm.h"
#include "lm.h"
#include "vocabulary.h"

using namespace std;

/////////////////////////
void push(DALM::VocabId *ngram, size_t n, DALM::VocabId wid){
	for(size_t i = n-1; i+1 >= 1 ; i--){
		ngram[i] = ngram[i-1];
	}
	ngram[0] = wid;
}

void read_ini(const char *inifile, string &model, string &words){
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
		}
		getline(ifs, line);
	}
}

/////////////////////////


namespace Moses
{
LanguageModelDALM::LanguageModelDALM(const std::string &line)
  :LanguageModelSingleFactor(line)
{
  ReadParameters();

  if (m_factorType == NOT_FOUND) {
    m_factorType = 0;
  }

  FactorCollection &factorCollection = FactorCollection::Instance();

  // needed by parent language model classes. Why didn't they set these themselves?
  m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, BOS_);
  m_sentenceStartWord[m_factorType] = m_sentenceStart;

  m_sentenceEnd		= factorCollection.AddFactor(Output, m_factorType, EOS_);
  m_sentenceEndWord[m_factorType] = m_sentenceEnd;
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
	read_ini(m_filePath.c_str(), model, words);

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
}

LMResult LanguageModelDALM::GetValue(const vector<const Word*> &contextFactor, State* finalState) const
{
  LMResult ret;

  // initialize DALM array
  DALM::VocabId ngram[m_nGramOrder];
  for(size_t i = 0; i < m_nGramOrder; i++){
	ngram[i] = wid_start;
  }

  DALM::VocabId wid;
  for (size_t i = 0; i < contextFactor.size(); ++i) {
	  const Word &word = *contextFactor[i];
	  wid = GetVocabId(word.GetFactor(m_factorType));
	  push(ngram, m_nGramOrder, wid);
  }

  // last word is unk?
  ret.unknown = (wid == DALM_UNK_WORD);

  // calc score. Doesn't handle unk yet
  float score = m_lm->query(ngram, m_nGramOrder);
  score = TransformLMScore(prob);
  ret.score = score;

  // use last word as state info
  const Factor *factor;
  size_t hash_value(const Factor &f);
  if (contextFactor.size()) {
    factor = contextFactor.back()->GetFactor(m_factorType);
  } else {
    factor = NULL;
  }

  (*finalState) = (State*) factor;

  return ret;
}

DALM::VocabId LanguageModelDALM::GetVocabId(const Factor *factor) const
{
	VocabMap::left_map::const_iterator iter;
	iter = m_vocabMap.left.find(factor);
	if (iter != m_vocabMap.left.end()) {
		return iter->second;
	}
	else {
		StringPiece str = factor->GetString();
		DALM::VocabId wid = m_vocab->lookup(str.as_string().c_str());
		return wid;
	}
}

}



