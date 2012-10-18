// For now, the individual features are not kept.  
#ifndef SEARCH_WEIGHTS__
#define SEARCH_WEIGHTS__

#include "search/types.hh"
#include "util/exception.hh"
#include "util/string_piece.hh"

#include <boost/unordered_map.hpp>

#include <string>

namespace search {

class WeightParseException : public util::Exception {
  public:
    WeightParseException() {}
    ~WeightParseException() throw() {}
};

class Weights {
  public:
    // Parses weights, sets lm_weight_, removes it from map_.
    explicit Weights(StringPiece text);

    // Just the three scores we care about adding.   
    Weights(Score lm, Score oov, Score word_penalty);

    Score DotNoLM(StringPiece text) const;

    Score LM() const { return lm_; }

    Score OOV() const { return oov_; }

    Score WordPenalty() const { return word_penalty_; }

    // Mostly for testing.  
    const boost::unordered_map<std::string, Score> &GetMap() const { return map_; }

  private:
    float Steal(const std::string &str);

    typedef boost::unordered_map<std::string, Score> Map;

    Map map_;

    Score lm_, oov_, word_penalty_;
};

} // namespace search

#endif // SEARCH_WEIGHTS__
