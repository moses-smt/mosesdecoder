#ifndef SEARCH_CONFIG__
#define SEARCH_CONFIG__

#include "search/weights.hh"
#include "util/string_piece.hh"

namespace search {

class Config {
  public:
    Config(const Weights &weights, unsigned int pop_limit) :
      weights_(weights), pop_limit_(pop_limit) {}

    const Weights &GetWeights() const { return weights_; }

    unsigned int PopLimit() const { return pop_limit_; }

  private:
    Weights weights_;
    unsigned int pop_limit_;
};

} // namespace search

#endif // SEARCH_CONFIG__
