#ifndef LM_LEFT__
#define LM_LEFT__

#include "lm/virtual_interface.hh"
#include "lm/max_order.hh"
#include "lm/model.hh"

namespace lm {
namespace ngram {

struct Left {
  bool operator==(const Left &other) const {
    return 
      (valid_length == other.valid_length) && 
      !memcmp(words, other.words, sizeof(WordIndex) * valid_length);
  }

  int Compare(const Left &other) const {
    if (valid_length != other.valid_length) {
      return (int)valid_length - (int)other.valid_length;
    }
    return memcmp(words, other.words, sizeof(WordIndex) * valid_length);
  }

  WordIndex words[kMaxOrder - 1];
  unsigned char valid_length;
};

struct ChartState {
  bool operator==(const ChartState &other) {
    return (left == other.left) && (right == other.right) && (small == other.small);
  }

  int Compare(const ChartState &other) const {
    int lres = left.Compare(other.left);
    if (lres) return lres;
    int rres = right.Compare(other.right);
    if (rres) return rres;
    return (int)small - (int)other.small;
  }

  Left left;
  State right;
  float left_est;
  bool small;
};

template <class M> class RuleScore {
  public:
    explicit RuleScore(const M &model, ChartState &out) : model_(model), out_(out), left_write_(out.left.words), left_end_(left_write_ + model.Order()), prob_(0.0) {
      out.left.valid_length = 0;
      out.right.valid_length_ = 0;
      out.left_est = 0.0;
      out.small = false;
    }

    void Terminal(WordIndex word) {
      float prob;
      if (word == model_.GetVocabulary().BeginSentence()) {
        out_.right = model_.BeginSentenceState();
        prob = 0.0;
      } else {
        State copy(out_.right);
        prob = model_.Score(copy, word, out_.right);
        prob_ += prob;
      }
      if (left_write_ < left_end_) {
        *(left_write_++) = word;
        out_.left_est += prob;
      }
    }

    void NonTerminal(const ChartState &in, float prob) {
      prob_ += prob - in.left_est;
      for (const WordIndex *i = in.left.words; i != in.left.words + in.left.valid_length; ++i) {
        Terminal(*i);
      }
      if (!in.small) out_.right = in.right;
    }

    float Finish() {
      out_.small = (left_write_ < left_end_);
      out_.left.valid_length = left_end_ - left_write_;
      return prob_;
    }

  private:
    const M &model_;

    ChartState &out_;

    WordIndex *left_write_;
    WordIndex *const left_end_;

    float prob_;
};

} // namespace ngram
} // namespace lm

#endif // LM_LEFT__
