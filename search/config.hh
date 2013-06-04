#ifndef SEARCH_CONFIG__
#define SEARCH_CONFIG__

#include "search/types.hh"

namespace search {

struct NBestConfig {
  explicit NBestConfig(unsigned int in_size) {
    keep = in_size;
    size = in_size;
  }
  
  unsigned int keep, size;
};

class Config {
  public:
    Config(Score lm_weight, unsigned int pop_limit, const NBestConfig &nbest) :
      lm_weight_(lm_weight), pop_limit_(pop_limit), nbest_(nbest) {}

    Score LMWeight() const { return lm_weight_; }

    unsigned int PopLimit() const { return pop_limit_; }

    const NBestConfig &GetNBest() const { return nbest_; }

  private:
    Score lm_weight_;

    unsigned int pop_limit_;

    NBestConfig nbest_;
};

} // namespace search

#endif // SEARCH_CONFIG__
