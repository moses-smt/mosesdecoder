#include "TargetNgramFeature.h"
#include "moses/Phrase.h"
#include "moses/TargetPhrase.h"
#include "moses/Hypothesis.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/ChartHypothesis.h"
#include "util/exception.hh"
#include "util/string_piece_hash.hh"

namespace Moses
{

using namespace std;

int TargetNgramState::Compare(const FFState& other) const
{
  const TargetNgramState& rhs = dynamic_cast<const TargetNgramState&>(other);
  int result;
  if (m_words.size() == rhs.m_words.size()) {
    for (size_t i = 0; i < m_words.size(); ++i) {
      result = Word::Compare(m_words[i],rhs.m_words[i]);
      if (result != 0) return result;
    }
    return 0;
  } else if (m_words.size() < rhs.m_words.size()) {
    for (size_t i = 0; i < m_words.size(); ++i) {
      result = Word::Compare(m_words[i],rhs.m_words[i]);
      if (result != 0) return result;
    }
    return -1;
  } else {
    for (size_t i = 0; i < rhs.m_words.size(); ++i) {
      result = Word::Compare(m_words[i],rhs.m_words[i]);
      if (result != 0) return result;
    }
    return 1;
  }
}

TargetNgramFeature::TargetNgramFeature(const std::string &line)
  :StatefulFeatureFunction(0, line)
{
  std::cerr << "Initializing target ngram feature.." << std::endl;

  ReadParameters();

  FactorCollection& factorCollection = FactorCollection::Instance();
  const Factor* bosFactor = factorCollection.AddFactor(Output,m_factorType,BOS_);
  m_bos.SetFactor(m_factorType,bosFactor);

  m_baseName = GetScoreProducerDescription();
  m_baseName.append("_");
}

void TargetNgramFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "factor") {
    m_factorType = Scan<FactorType>(value);
  } else if (key == "n") {
    m_n = Scan<size_t>(value);
  } else if (key == "lower-ngrams") {
    m_lower_ngrams = Scan<bool>(value);
  } else if (key == "file") {
    m_file = value;
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

void TargetNgramFeature::Load()
{
  if (m_file == "") return; //allow all, for now

  if (m_file == "*") return; //allow all
  ifstream inFile(m_file.c_str());
  if (!inFile) {
    UTIL_THROW(util::Exception, "Couldn't open file" << m_file);
  }

  std::string line;
  m_vocab.insert(BOS_);
  m_vocab.insert(EOS_);
  while (getline(inFile, line)) {
    m_vocab.insert(line);
    cerr << "ADD TO VOCAB: '" << line << "'" << endl;
  }

  inFile.close();
  return;
}

const FFState* TargetNgramFeature::EmptyHypothesisState(const InputType &/*input*/) const
{
  vector<Word> bos(1,m_bos);
  return new TargetNgramState(bos);
}

FFState* TargetNgramFeature::EvaluateWhenApplied(const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const
{
  const TargetNgramState* tnState = static_cast<const TargetNgramState*>(prev_state);
  assert(tnState);

  // current hypothesis target phrase
  const Phrase& targetPhrase = cur_hypo.GetCurrTargetPhrase();
  if (targetPhrase.GetSize() == 0) return new TargetNgramState(*tnState);

  // extract all ngrams from current hypothesis
  vector<Word> prev_words(tnState->GetWords());
  stringstream curr_ngram;
  bool skip = false;

  // include lower order ngrams?
  size_t smallest_n = m_n;
  if (m_lower_ngrams) smallest_n = 1;

  for (size_t n = m_n; n >= smallest_n; --n) { // iterate over ngram size
    for (size_t i = 0; i < targetPhrase.GetSize(); ++i) {
//  		const string& curr_w = targetPhrase.GetWord(i).GetFactor(m_factorType)->GetString();
      const StringPiece curr_w = targetPhrase.GetWord(i).GetString(m_factorType);

      //cerr << "CHECK WORD '" << curr_w << "'" << endl;
      if (m_vocab.size() && (FindStringPiece(m_vocab, curr_w) == m_vocab.end())) continue; // skip ngrams
      //cerr << "ALLOWED WORD '" << curr_w << "'" << endl;

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
        curr_ngram << curr_w;
        //cerr << "SCORE '" << curr_ngram.str() << "'" << endl;
        accumulator->PlusEquals(this,curr_ngram.str(),1);
      }
      curr_ngram.str("");
    }
  }

  if (cur_hypo.GetWordsBitmap().IsComplete()) {
    for (size_t n = m_n; n >= smallest_n; --n) {
      stringstream last_ngram;
      skip = false;
      for (size_t i = cur_hypo.GetSize() - n + 1; i <  cur_hypo.GetSize() && !skip; ++i)
        appendNgram(cur_hypo.GetWord(i), skip, last_ngram);

      if (n > 1 && !skip) {
        last_ngram << EOS_;
        accumulator->PlusEquals(this, last_ngram.str(), 1);
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
  } else {
    // take words from previous state and from target phrase
    int from_prev_state = m_n - 1 - targetPhrase.GetSize();
    for (size_t i = prev_words.size()-from_prev_state; i < prev_words.size(); ++i)
      new_prev_words.push_back(prev_words[i]);
    for (size_t i = 0; i < targetPhrase.GetSize(); ++i)
      new_prev_words.push_back(targetPhrase.GetWord(i));
  }
  return new TargetNgramState(new_prev_words);
}

void TargetNgramFeature::appendNgram(const Word& word, bool& skip, stringstream &ngram) const
{
//	const string& w = word.GetFactor(m_factorType)->GetString();
  const StringPiece w = word.GetString(m_factorType);
  if (m_vocab.size() && (FindStringPiece(m_vocab, w) == m_vocab.end())) skip = true;
  else {
    ngram << w;
    ngram << ":";
  }
}

FFState* TargetNgramFeature::EvaluateWhenApplied(const ChartHypothesis& cur_hypo, int featureId, ScoreComponentCollection* accumulator) const
{
  vector<const Word*> contextFactor;
  contextFactor.reserve(m_n);

  // get index map for underlying hypotheses
  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
    cur_hypo.GetCurrTargetPhrase().GetAlignNonTerm().GetNonTermIndexMap();

  // loop over rule
  bool makePrefix = false;
  bool makeSuffix = false;
  bool collectForPrefix = true;
  size_t prefixTerminals = 0;
  size_t suffixTerminals = 0;
  bool onlyTerminals = true;
  bool prev_is_NT = false;
  size_t prev_subPhraseLength = 0;
  for (size_t phrasePos = 0; phrasePos < cur_hypo.GetCurrTargetPhrase().GetSize(); phrasePos++) {
    // consult rule for either word or non-terminal
    const Word &word = cur_hypo.GetCurrTargetPhrase().GetWord(phrasePos);
//    cerr << "word: " << word << endl;

    // regular word
    if (!word.IsNonTerminal()) {
      contextFactor.push_back(&word);
      prev_is_NT = false;

      if (phrasePos==0)
        makePrefix = true;
      if (phrasePos==cur_hypo.GetCurrTargetPhrase().GetSize()-1 || prev_is_NT)
        makeSuffix = true;

      // beginning/end of sentence symbol <s>,</s>?
      StringPiece factorZero = word.GetString(0);
      if (factorZero.compare("<s>") == 0)
        prefixTerminals++;
      // end of sentence symbol </s>?
      else if (factorZero.compare("</s>") == 0)
        suffixTerminals++;
      // everything else
      else {
        stringstream ngram;
        ngram << m_baseName;
        if (m_factorType == 0)
          ngram << factorZero;
        else
          ngram << word.GetString(m_factorType);
        accumulator->SparsePlusEquals(ngram.str(), 1);

        if (collectForPrefix)
          prefixTerminals++;
        else
          suffixTerminals++;
      }
    }

    // non-terminal, add phrase from underlying hypothesis
    else if (m_n > 1) {
      // look up underlying hypothesis
      size_t nonTermIndex = nonTermIndexMap[phrasePos];
      const ChartHypothesis *prevHypo = cur_hypo.GetPrevHypo(nonTermIndex);

      const TargetNgramChartState* prevState =
        static_cast<const TargetNgramChartState*>(prevHypo->GetFFState(featureId));
      size_t subPhraseLength = prevState->GetNumTargetTerminals();

      // special case: rule starts with non-terminal
      if (phrasePos == 0) {
        if (subPhraseLength == 1) {
          makePrefix = true;
          ++prefixTerminals;

          const Word &word = prevState->GetSuffix().GetWord(0);
//      		cerr << "NT0 --> : " << word << endl;
          contextFactor.push_back(&word);
        } else {
          onlyTerminals = false;
          collectForPrefix = false;
          int suffixPos = prevState->GetSuffix().GetSize() - (m_n-1);
          if (suffixPos < 0) suffixPos = 0; // push all words if less than order
          for(; (size_t)suffixPos < prevState->GetSuffix().GetSize(); suffixPos++) {
            const Word &word = prevState->GetSuffix().GetWord(suffixPos);
//      			cerr << "NT0 --> : " << word << endl;
            contextFactor.push_back(&word);
          }
        }
      }

      // internal non-terminal
      else {
        // push its prefix
        for(size_t prefixPos = 0; prefixPos < m_n-1
            && prefixPos < subPhraseLength; prefixPos++) {
          const Word &word = prevState->GetPrefix().GetWord(prefixPos);
//          cerr << "NT --> " << word << endl;
          contextFactor.push_back(&word);
        }

        if (subPhraseLength==1) {
          if (collectForPrefix)
            ++prefixTerminals;
          else
            ++suffixTerminals;

          if (phrasePos == cur_hypo.GetCurrTargetPhrase().GetSize()-1)
            makeSuffix = true;
        } else {
          onlyTerminals = false;
          collectForPrefix = true;

          // check if something follows this NT
          bool wordFollowing = (phrasePos < cur_hypo.GetCurrTargetPhrase().GetSize() - 1)? true : false;

          // check if we are dealing with a large sub-phrase
          if (wordFollowing && subPhraseLength > m_n - 1) {
            // clear up pending ngrams
            MakePrefixNgrams(contextFactor, accumulator, prefixTerminals);
            contextFactor.clear();
            makePrefix = false;
            makeSuffix = true;
            collectForPrefix = false;
            prefixTerminals = 0;
            suffixTerminals = 0;

            // push its suffix
            size_t remainingWords = (remainingWords > m_n-1) ? m_n-1 : subPhraseLength - (m_n-1);
            for(size_t suffixPos = 0; suffixPos < prevState->GetSuffix().GetSize(); suffixPos++) {
              const Word &word = prevState->GetSuffix().GetWord(suffixPos);
//      				cerr << "NT --> : " << word << endl;
              contextFactor.push_back(&word);
            }
          }
          // subphrase can be used as suffix and as prefix for the next part
          else if (wordFollowing && subPhraseLength == m_n - 1) {
            // clear up pending ngrams
            MakePrefixNgrams(contextFactor, accumulator, prefixTerminals);
            makePrefix = false;
            makeSuffix = true;
            collectForPrefix = false;
            prefixTerminals = 0;
            suffixTerminals = 0;
          } else if (prev_is_NT && prev_subPhraseLength > 1 && subPhraseLength > 1) {
            // two NTs in a row: make transition
            MakePrefixNgrams(contextFactor, accumulator, 1, m_n-2);
            MakeSuffixNgrams(contextFactor, accumulator, 1, m_n-2);
            makePrefix = false;
            makeSuffix = false;
            collectForPrefix = false;
            prefixTerminals = 0;
            suffixTerminals = 0;

            // remove duplicates
            stringstream curr_ngram;
            curr_ngram << m_baseName;
            curr_ngram << (*contextFactor[m_n-2]).GetString(m_factorType);
            curr_ngram << ":";
            curr_ngram << (*contextFactor[m_n-1]).GetString(m_factorType);
            accumulator->SparseMinusEquals(curr_ngram.str(),1);
          }
        }
      }
      prev_is_NT = true;
      prev_subPhraseLength = subPhraseLength;
    }
  }

  if (m_n > 1) {
    if (onlyTerminals) {
      MakePrefixNgrams(contextFactor, accumulator, prefixTerminals-1);
    } else {
      if (makePrefix)
        MakePrefixNgrams(contextFactor, accumulator, prefixTerminals);
      if (makeSuffix)
        MakeSuffixNgrams(contextFactor, accumulator, suffixTerminals);

      // remove duplicates
      size_t size = contextFactor.size();
      if (makePrefix && makeSuffix && (size <= m_n)) {
        stringstream curr_ngram;
        curr_ngram << m_baseName;
        for (size_t i = 0; i < size; ++i) {
          curr_ngram << (*contextFactor[i]).GetString(m_factorType);
          if (i < size-1)
            curr_ngram << ":";
        }
        accumulator->SparseMinusEquals(curr_ngram.str(), 1);
      }
    }
  }

//  cerr << endl;
  return new TargetNgramChartState(cur_hypo, featureId, m_n);
}

void TargetNgramFeature::MakePrefixNgrams(std::vector<const Word*> &contextFactor, ScoreComponentCollection* accumulator, size_t numberOfStartPos, size_t offset) const
{
  stringstream ngram;
  size_t size = contextFactor.size();
  for (size_t k = 0; k < numberOfStartPos; ++k) {
    size_t max_end = (size < m_n+k+offset)? size: m_n+k+offset;
    for (size_t end_pos = 1+k+offset; end_pos < max_end; ++end_pos) {
      ngram << m_baseName;
      for (size_t i=k+offset; i <= end_pos; ++i) {
        if (i > k+offset)
          ngram << ":";
        StringPiece factorZero = (*contextFactor[i]).GetString(0);
        if (m_factorType == 0 || factorZero.compare("<s>") == 0 || factorZero.compare("</s>") == 0)
          ngram << factorZero;
        else
          ngram << (*contextFactor[i]).GetString(m_factorType);
        const Word w = *contextFactor[i];
      }
//      cerr << "p-ngram: " << ngram.str() << endl;
      accumulator->SparsePlusEquals(ngram.str(), 1);
      ngram.str("");
    }
  }
}

void TargetNgramFeature::MakeSuffixNgrams(std::vector<const Word*> &contextFactor, ScoreComponentCollection* accumulator, size_t numberOfEndPos, size_t offset) const
{
  stringstream ngram;
  for (size_t k = 0; k < numberOfEndPos; ++k) {
    size_t end_pos = contextFactor.size()-1-k-offset;
    for (int start_pos=end_pos-1; (start_pos >= 0) && (end_pos-start_pos < m_n); --start_pos) {
      ngram << m_baseName;
      for (size_t j=start_pos; j <= end_pos; ++j) {
        StringPiece factorZero = (*contextFactor[j]).GetString(0);
        if (m_factorType == 0 || factorZero.compare("<s>") == 0 || factorZero.compare("</s>") == 0)
          ngram << factorZero;
        else
          ngram << (*contextFactor[j]).GetString(m_factorType);
        if (j < end_pos)
          ngram << ":";
      }
//      cerr << "s-ngram: " << ngram.str() << endl;
      accumulator->SparsePlusEquals(ngram.str(), 1);
      ngram.str("");
    }
  }
}

bool TargetNgramFeature::IsUseable(const FactorMask &mask) const
{
  bool ret = mask[m_factorType];
  return ret;
}

}

