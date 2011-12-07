#include "TargetNgramFeature.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Hypothesis.h"
#include "ScoreComponentCollection.h"
#include "ChartHypothesis.h"

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
  m_vocab.insert(EOS_);
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
  assert(tnState);

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

FFState* TargetNgramFeature::EvaluateChart(const ChartHypothesis& cur_hypo, int featureID, ScoreComponentCollection* accumulator) const
{
	TargetNgramChartState *ret = new TargetNgramChartState(cur_hypo, featureID, GetNGramOrder());
	// data structure for factored context phrase (history and predicted word)
  vector<const Word*> contextFactor;
  contextFactor.reserve(GetNGramOrder());

  // initialize language model context state
  FFState *lmState = NewState( GetNullContextState() );

  // get index map for underlying hypotheses
  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
    cur_hypo.GetCurrTargetPhrase().GetAlignmentInfo().GetNonTermIndexMap();

  // loop over rule
  bool makePrefix = false;
  bool makeSuffix = false;
  bool beforeSubphrase = true;
  size_t terminalsBeforeSubphrase = 0;
  size_t terminalsAfterSubphrase = 0;
  for (size_t phrasePos = 0, wordPos = 0;
       phrasePos < cur_hypo.GetCurrTargetPhrase().GetSize();
       phrasePos++)
  {
    // consult rule for either word or non-terminal
    const Word &word = cur_hypo.GetCurrTargetPhrase().GetWord(phrasePos);
//    cerr << "word: " << word << endl;

    // regular word
    if (!word.IsNonTerminal())
    {
    	if (phrasePos==0)
    		makePrefix = true;

    	contextFactor.push_back(&word);

      // beginning of sentence symbol <s>?
      if (word.GetString(GetFactorType(), false).compare("<s>") == 0)
      {
      	assert(phrasePos == 0);
      	delete lmState;
        lmState = NewState( GetBeginSentenceState() );

        terminalsBeforeSubphrase++;
      }
      // end of sentence symbol </s>?
      else if (word.GetString(GetFactorType(), false).compare("</s>") == 0) {
      	terminalsAfterSubphrase++;
      }
      // everything else
      else {
      	string curr_ngram = word.GetString(GetFactorType(), false);
//    	cerr << "ngram: " << curr_ngram << endl;
      	accumulator->PlusEquals(this,curr_ngram,1);
      }

    }

    // non-terminal, add phrase from underlying hypothesis
    else if (GetNGramOrder() > 1)
    {
      // look up underlying hypothesis
      size_t nonTermIndex = nonTermIndexMap[phrasePos];
      const ChartHypothesis *prevHypo = cur_hypo.GetPrevHypo(nonTermIndex);

      const TargetNgramChartState* prevState =
        static_cast<const TargetNgramChartState*>(prevHypo->GetFFState(featureID));

      size_t subPhraseLength = prevState->GetNumTargetTerminals();
      if (subPhraseLength==1) {
      	if (beforeSubphrase)
      		terminalsBeforeSubphrase++;
      	else
      		terminalsAfterSubphrase++;
      }
      else {
      	beforeSubphrase = false;
      }

      // special case: rule starts with non-terminal -> copy everything
      if (phrasePos == 0) {
      	if (subPhraseLength == 1)
      		makePrefix = true;

        // get language model state
      	delete lmState;
        lmState = NewState( prevState->GetRightContext() );

        // push suffix
//        cerr << "suffix of NT in the beginning" << endl;
        int suffixPos = prevState->GetSuffix().GetSize() - (GetNGramOrder()-1);
        if (suffixPos < 0) suffixPos = 0; // push all words if less than order
        for(;(size_t)suffixPos < prevState->GetSuffix().GetSize(); suffixPos++)
        {
          const Word &word = prevState->GetSuffix().GetWord(suffixPos);
//          cerr << "NT0 --> : " << word << endl;
          contextFactor.push_back(&word);
          wordPos++;
        }
      }

      // internal non-terminal
      else
      {
      	if (subPhraseLength == 1 && phrasePos == cur_hypo.GetCurrTargetPhrase().GetSize()-1)
      		makeSuffix = true;

//      	cerr << "prefix of subphrase for left context" << endl;
        // score its prefix
        for(size_t prefixPos = 0;
            prefixPos < GetNGramOrder()-1 // up to LM order window
              && prefixPos < subPhraseLength; // up to length
            prefixPos++)
        {
          const Word &word = prevState->GetPrefix().GetWord(prefixPos);
//          cerr << "NT --> " << word << endl;
          contextFactor.push_back(&word);
        }

        bool next = false;
        if (phrasePos < cur_hypo.GetCurrTargetPhrase().GetSize() - 1)
        	next = true;

        // check if we are dealing with a large sub-phrase
        if (next && subPhraseLength > GetNGramOrder() - 1) // TODO: CHECK??
        {
        	// clear up pending ngrams
        	MakePrefixNgrams(contextFactor, accumulator, terminalsBeforeSubphrase);
        	contextFactor.clear();
        	makePrefix = false;
        	makeSuffix = true;
//        	cerr << "suffix of subphrase for right context (only if something is following)" << endl;

          // copy language model state
          delete lmState;
          lmState = NewState( prevState->GetRightContext() );

          // push its suffix
          size_t remainingWords = subPhraseLength - (GetNGramOrder()-1);
          if (remainingWords > GetNGramOrder()-1) {
            // only what is needed for the history window
            remainingWords = GetNGramOrder()-1;
          }
          for(size_t suffixPos = 0; suffixPos < prevState->GetSuffix().GetSize(); suffixPos++) {
            const Word &word = prevState->GetSuffix().GetWord(suffixPos);
//            cerr << "NT --> : " << word << endl;
            contextFactor.push_back(&word);
          }
          wordPos += subPhraseLength;
        }
      }
    }
  }

  if (GetNGramOrder() > 1) {
  	if (makePrefix) {
  		size_t terminals = beforeSubphrase? 1 : terminalsBeforeSubphrase;
  		MakePrefixNgrams(contextFactor, accumulator, terminals);
  	}
  	if (makeSuffix) {
  		size_t terminals = beforeSubphrase? 1 : terminalsAfterSubphrase;
  		MakeSuffixNgrams(contextFactor, accumulator, terminals);
  	}

  	// remove duplicates
  	if (makePrefix && makeSuffix && (contextFactor.size() <= GetNGramOrder())) {
  		string curr_ngram;
  		for (size_t i = 0; i < contextFactor.size(); ++i) {
  			curr_ngram.append((*contextFactor[i]).GetString(GetFactorType(), false));
  			if (i < contextFactor.size()-1)
  				curr_ngram.append(":");
  		}
  		accumulator->MinusEquals(this,curr_ngram,1);
  	}
  }

  ret->Set(lmState);
//  cerr << endl;
  return ret;
}

void TargetNgramFeature::ShiftOrPush(std::vector<const Word*> &contextFactor, const Word &word) const
{
  if (contextFactor.size() < GetNGramOrder()) {
    contextFactor.push_back(&word);
  } else {
    // shift
    for (size_t currNGramOrder = 0 ; currNGramOrder < GetNGramOrder() - 1 ; currNGramOrder++) {
      contextFactor[currNGramOrder] = contextFactor[currNGramOrder + 1];
    }
    contextFactor[GetNGramOrder() - 1] = &word;
  }
}

void TargetNgramFeature::MakePrefixNgrams(std::vector<const Word*> &contextFactor, ScoreComponentCollection* accumulator, size_t numberOfStartPos) const {
	string curr_ngram;
	size_t size = contextFactor.size();
	for (size_t k = 0; k < numberOfStartPos; ++k) {
		size_t max_length = (size < GetNGramOrder())? size: GetNGramOrder();
		for (size_t end = 1+k; end < max_length+k; ++end) {
			for (size_t i=k; i <= end; ++i) {
				if (i > k)
					curr_ngram.append(":");
				curr_ngram.append((*contextFactor[i]).GetString(GetFactorType(), false));
			}
			if (curr_ngram != "<s>" && curr_ngram != "</s>") {
//				cerr << "p-ngram: " << curr_ngram << endl;
				accumulator->PlusEquals(this,curr_ngram,1);
			}
			curr_ngram.clear();
		}
	}
}

void TargetNgramFeature::MakeSuffixNgrams(std::vector<const Word*> &contextFactor, ScoreComponentCollection* accumulator, size_t numberOfEndPos) const {
	string curr_ngram;
	size_t size = contextFactor.size();
	for (size_t k = 0; k < numberOfEndPos; ++k) {
		size_t min_start = (size > GetNGramOrder())? (size - GetNGramOrder()): 0;
		size_t end = size-1;
		for (size_t start=min_start-k; start < end-k; ++start) {
			for (size_t j=start; j < size-k; ++j){
				curr_ngram.append((*contextFactor[j]).GetString(GetFactorType(), false));
				if (j < size-k-1)
					curr_ngram.append(":");
			}
			if (curr_ngram != "<s>" && curr_ngram != "</s>") {
//				cerr << "s-ngram: " << curr_ngram << endl;
				accumulator->PlusEquals(this,curr_ngram,1);
			}
			curr_ngram.clear();
		}
	}
}

bool TargetNgramFeature::Load(const std::string &filePath, FactorType factorType, size_t nGramOrder) {
	// dummy
	cerr << "This method has not been implemented.." << endl;
	assert(false);
	return false;
}

LMResult TargetNgramFeature::GetValue(const std::vector<const Word*> &contextFactor, State* finalState) const {
	// dummy
	LMResult* result = new LMResult();
	cerr << "This method has not been implemented.." << endl;
	assert(false);
	return *result;
}

}

