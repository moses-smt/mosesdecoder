#include "TargetNgramFeature.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Hypothesis.h"
#include "ScoreComponentCollection.h"

namespace Moses {

using namespace std;

int TargetNgramState::Compare(const FFState& other) const {
  const TargetNgramState& rhs = dynamic_cast<const TargetNgramState&>(other);
  int result;
  if (m_words.size() == rhs.m_words.size()) {
  	for (size_t i = 0; i < m_words.size(); ++i) {
  		result = Word::Compare(m_words[i],rhs.m_words[i]);
  		if (result != 0) return result;
  	}
    return 0;
  }
  else if (m_words.size() < rhs.m_words.size()) {
  	for (size_t i = 0; i < m_words.size(); ++i) {
  		result = Word::Compare(m_words[i],rhs.m_words[i]);
  		if (result != 0) return result;
  	}
  	return -1;
  }
  else {
  	for (size_t i = 0; i < rhs.m_words.size(); ++i) {
  		result = Word::Compare(m_words[i],rhs.m_words[i]);
  		if (result != 0) return result;
  	}
  	return 1;
  }
}

bool TargetNgramFeature::Load(const std::string &filePath)
{
  if (filePath == "*") return true; //allow all
  ifstream inFile(filePath.c_str());
  if (!inFile)
  {
      return false;
  }

  std::string line;
  m_vocab.insert(BOS_);
  m_vocab.insert(BOS_);
  while (getline(inFile, line)) {
    m_vocab.insert(line);
  }

  inFile.close();
  return true;
}


string TargetNgramFeature::GetScoreProducerWeightShortName(unsigned) const
{
	return "dlmn";
}

size_t TargetNgramFeature::GetNumInputScores() const
{
	return 0;
}


const FFState* TargetNgramFeature::EmptyHypothesisState(const InputType &/*input*/) const
{
	vector<Word> bos(1,m_bos);
  return new TargetNgramState(bos);
}

FFState* TargetNgramFeature::Evaluate(const Hypothesis& cur_hypo,
                                       const FFState* prev_state,
                                       ScoreComponentCollection* accumulator) const
{
  const TargetNgramState* tnState = dynamic_cast<const TargetNgramState*>(prev_state);
  CHECK(tnState);

  // current hypothesis target phrase
  const Phrase& targetPhrase = cur_hypo.GetCurrTargetPhrase();
  if (targetPhrase.GetSize() == 0) return new TargetNgramState(*tnState);

  // extract all ngrams from current hypothesis
  vector<Word> prev_words = tnState->GetWords();
  string curr_ngram;
  bool skip = false;

  // include lower order ngrams?
  size_t smallest_n = m_n;
  if (m_lower_ngrams) smallest_n = 1;

  for (size_t n = m_n; n >= smallest_n; --n) { // iterate over ngram size
  	for (size_t i = 0; i < targetPhrase.GetSize(); ++i) {
  		const string& curr_w = targetPhrase.GetWord(i).GetFactor(m_factorType)->GetString();
  		if (m_vocab.size() && (m_vocab.find(curr_w) == m_vocab.end())) continue; // skip ngrams

  		if (n > 1) {
  			// can we build an ngram at this position? ("<s> this" --> cannot build 3gram at this position)
  			size_t pos_in_translation = cur_hypo.GetSize() - targetPhrase.GetSize() + i;
  			if (pos_in_translation < n - 2) continue; // need at least m_n - 1 words

  			// how many words needed from previous state?
  			int from_prev_state = n - (i+1);
  			skip = false;
  			if (from_prev_state > 0) {
  				if (prev_words.size() < from_prev_state) {
  					// context is too short, make new state from previous state and target phrase
  					vector<Word> new_prev_words;
  					for (size_t i = 0; i < prev_words.size(); ++i)
  						new_prev_words.push_back(prev_words[i]);
        	  for (size_t i = 0; i < targetPhrase.GetSize(); ++i)
        	  	new_prev_words.push_back(targetPhrase.GetWord(i));
        	 	return new TargetNgramState(new_prev_words);
  				}

  				// add words from previous state
  				for (size_t j = prev_words.size()-from_prev_state; j < prev_words.size() && !skip; ++j)
  					appendNgram(prev_words[j], skip, curr_ngram);
        }

  			// add words from current target phrase
  			int start = i - n + 1; // add m_n-1 previous words
  			if (start < 0) start = 0; // or less
  			for (size_t j = start; j < i && !skip; ++j)
  				appendNgram(targetPhrase.GetWord(j), skip, curr_ngram);
      }

  		if (!skip) {
  			curr_ngram.append(curr_w);
  			accumulator->PlusEquals(this,curr_ngram,1);
      }
  		curr_ngram.clear();
  	}
  }

  if (cur_hypo.GetWordsBitmap().IsComplete()) {
  	for (size_t n = m_n; n >= smallest_n; --n) {
  		string last_ngram;
  		skip = false;
  		for (size_t i = cur_hypo.GetSize() - n + 1; i <  cur_hypo.GetSize() && !skip; ++i)
  			appendNgram(cur_hypo.GetWord(i), skip, last_ngram);

  		if (n > 1 && !skip) {
  			last_ngram.append(EOS_);
  			accumulator->PlusEquals(this,last_ngram,1);
    	}
  	}
  	return NULL;
  }

  // prepare new state
  vector<Word> new_prev_words;
  if (targetPhrase.GetSize() >= m_n-1) {
  	// take subset of target words
  	for (size_t i = targetPhrase.GetSize() - m_n + 1; i < targetPhrase.GetSize(); ++i)
  		new_prev_words.push_back(targetPhrase.GetWord(i));
  }
  else {
  	// take words from previous state and from target phrase
  	int from_prev_state = m_n - 1 - targetPhrase.GetSize();
  	for (size_t i = prev_words.size()-from_prev_state; i < prev_words.size(); ++i)
  		new_prev_words.push_back(prev_words[i]);
  	for (size_t i = 0; i < targetPhrase.GetSize(); ++i)
  		new_prev_words.push_back(targetPhrase.GetWord(i));
  }
  return new TargetNgramState(new_prev_words);
}

void TargetNgramFeature::appendNgram(const Word& word, bool& skip, string& ngram) const {
	const string& w = word.GetFactor(m_factorType)->GetString();
	if (m_vocab.size() && (m_vocab.find(w) == m_vocab.end())) skip = true;
	else {
		ngram.append(w);
		ngram.append(":");
	}
}
}

