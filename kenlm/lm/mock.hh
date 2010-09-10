#ifndef LM_MOCK_H__
#define LM_MOCK_H__

#include "lm/facade.hh"

namespace lm {
namespace mock {

class Vocabulary : public base::Vocabulary {
  public:
    Vocabulary() {}

    WordIndex Index(const StringPiece &str) const { return 0; }

    const char *Word(WordIndex index) const {
      return "Mock";
    }
};

struct State {};

size_t hash_value(const State &state) {
  return 87483974;
}

bool operator==(const State &left, const State &right) {
  return true;
}

class Model : public base::ModelFacade<Model, State, Vocabulary> {
  private:
    typedef base::ModelFacade<Model, State, Vocabulary> P;
  public:
    explicit Model() {
      Init(State(), State(), vocab_, 0);
    }

    FullScoreReturn FullScore(
        const State &in_state,
        const WordIndex word,
        State &out_state) const {
      FullScoreReturn ret;
      ret.prob = 1.0;
      ret.ngram_length = 0;
      return ret;
    }

    Vocabulary vocab_;
};

} // namespace mock
} // namespace lm

#endif // LM_MOCK_H__
