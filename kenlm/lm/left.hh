#ifndef LM_LEFT__
#define LM_LEFT__

#include "lm/virtual_interface.hh"
#include "lm/max_order.hh"
#include "lm/model.hh"

namespace lm {
namespace ngram {

struct Left {
  bool operator==(const Left &other_) const {
    return 
      (valid_length_ == other_.valid_length_) && 
      !memcmp(words_, other_.words_, sizeof(WordIndex) * valid_length_);
  }

  int Compare(const Left &other_) const {
    if (valid_length_ != other_.valid_length_) {
      return (int)valid_length_ - (int)other_.valid_length_;
    }
    return memcmp(words_, other_.words_, sizeof(WordIndex) * valid_length_);
  }

  WordIndex words_[kMaxOrder - 1];
  unsigned char valid_length_;
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
    explicit RuleScore(const M &model, ChartState &out) : model_(model), out_(out), left_write_(out.left.words_), left_end_(left_write_ + model.Order()), prob_(0.0) {
      out.left.valid_length_ = 0;
      out.right.valid_length_ = 0;
      out.left_est = 0.0;
      out.small = false;
    }

    void Terminal(WordIndex word) {
      State copy(out_.right);
      float prob = model_.Score(copy, word, out_.right);
      prob_ += prob;
      if (left_write_ < left_end_) {
        *(left_write_++) = word;
        out_.left_est += prob;
      }
    }

    void Nonterminal(const ChartState &in, float prob) {
      prob_ += prob - in.left_est;
      for (const WordIndex *i = in.left.words_; i != in.left.words_ + in.valid_length; ++i) {
        Terminal(*i);
      }
      if (!in.small) out_.right = in.right;
    }

    float Finish() const {
      out_.small = (left_write_ < left_end_);
      return prob_;
    }

  private:
    M &model_;

    ChartState &out_;

    WordIndex *left_write_;
    const WordIndex *const left_end_;

    float prob_;
};

} // namespace ngram
} // namespace lm

#endif // LM_LEFT__
