#include "lm/exception.hh"
#include "lm/sri.hh"

#include <Ngram.h>
#include <Vocab.h>

#include <iostream>

namespace lm {
namespace sri {

Vocabulary::Vocabulary() : sri_(new Vocab) {}

Vocabulary::~Vocabulary() {}

WordIndex Vocabulary::Index(const char *str) const {
  WordIndex ret = sri_->getIndex(str);
  // NGram wants the index of Vocab_Unknown for unknown words, but for some reason SRI returns Vocab_None here :-(.
  if (ret == Vocab_None) {
    return not_found_;
  } else {
    return ret;
  }
}

const char *Vocabulary::Word(WordIndex index) const {
  return sri_->getWord(index);
}

void Vocabulary::FinishedLoading() {
  SetSpecial(
    sri_->getIndex(Vocab_SentStart),
    sri_->getIndex(Vocab_SentEnd),
    sri_->getIndex(Vocab_Unknown),
    sri_->highIndex() + 1);
}

namespace {
Ngram *MakeSRIModel(const char *file_name, unsigned int ngram_length, Vocab &sri_vocab) throw (ReadFileLoadException) {
  sri_vocab.unkIsWord() = true;
  std::auto_ptr<Ngram> ret(new Ngram(sri_vocab, ngram_length));
  File file(file_name, "r");
  if (!ret->read(file)) {
    throw ReadFileLoadException(file_name);
  }
  return ret.release();
}
} // namespace

Model::Model(const char *file_name, unsigned int ngram_length) : sri_(MakeSRIModel(file_name, ngram_length, *vocab_.sri_)) {
  // TODO: exception this?
  if (!sri_->setorder()) {
    std::cerr << "Can't have order 0 SRI" << std::endl;
    abort();
  }
  State begin_state = State();
  begin_state.valid_length_ = 1;
  begin_state.history_[0] = vocab_.BeginSentence();
  State null_state = State();
  null_state.valid_length_ = 0;
  Init(begin_state, null_state, vocab_, sri_->setorder());
  not_found_ = vocab_.NotFound();
}

Model::~Model() {}

FullScoreReturn Model::FullScore(const State &in_state, const WordIndex new_word, State &out_state) const {
  // If you get a compiler in this function, change SRIVocabIndex in sri.hh to match the one found in SRI's Vocab.h.
  // TODO: optimize this to use the new state's history.  
  SRIVocabIndex history[Order()];
  std::copy(in_state.history_, in_state.history_ + in_state.valid_length_, history);
  history[in_state.valid_length_] = Vocab_None;
  const SRIVocabIndex *const_history = history;
  FullScoreReturn ret;
  // TODO: avoid double backoff.
  if (new_word != not_found_) {
    // This gets the length of context used, which is ngram_length - 1 unless new_word is OOV in which case it is 0.
    unsigned int out_length = 0;
    sri_->contextID(new_word, const_history, out_length);
    ret.ngram_length = out_length + 1;
    out_state.history_[0] = new_word;
    out_state.valid_length_ = std::min<unsigned char>(ret.ngram_length, Order() - 1);
    std::copy(history, history + out_state.valid_length_ - 1, out_state.history_ + 1);
  } else {
    ret.ngram_length = 0;
    out_state.valid_length_ = 0;
  }
  ret.prob = sri_->wordProb(new_word, const_history);
  // SRI uses log10, we use log.
  return ret;
}

} // namespace sri
} // namespace lm
