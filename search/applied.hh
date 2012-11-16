#ifndef SEARCH_APPLIED__
#define SEARCH_APPLIED__

#include "search/edge.hh"
#include "search/header.hh"
#include "util/pool.hh"

#include <math.h>

namespace search {

// A full hypothesis: a score, arity of the rule, a pointer to the decoder's rule (Note), and pointers to non-terminals that were substituted.  
template <class Below> class GenericApplied : public Header {
  public:
    GenericApplied() {}

    GenericApplied(void *location, PartialEdge partial) 
      : Header(location) {
      memcpy(Base(), partial.Base(), kHeaderSize);
      Below *child_out = Children();
      const PartialVertex *part = partial.NT();
      const PartialVertex *const part_end_loop = part + partial.GetArity();
      for (; part != part_end_loop; ++part, ++child_out)
        *child_out = Below(part->End());
    }
    
    GenericApplied(void *location, Score score, Arity arity, Note note) : Header(location, arity) {
      SetScore(score);
      SetNote(note);
    }

    explicit GenericApplied(History from) : Header(from) {}


    // These are arrays of length GetArity().
    Below *Children() {
      return reinterpret_cast<Below*>(After());
    }
    const Below *Children() const {
      return reinterpret_cast<const Below*>(After());
    }

    static std::size_t Size(Arity arity) {
      return kHeaderSize + arity * sizeof(const Below);
    }
};

// Applied rule that references itself.  
class Applied : public GenericApplied<Applied> {
  private:
    typedef GenericApplied<Applied> P;

  public:
    Applied() {}
    Applied(void *location, PartialEdge partial) : P(location, partial) {}
    Applied(History from) : P(from) {}
};

// How to build single-best hypotheses.  
class SingleBest {
  public:
    typedef PartialEdge Combine;

    void Add(PartialEdge &existing, PartialEdge add) const {
      if (!existing.Valid() || existing.GetScore() < add.GetScore())
        existing = add;
    }

    NBestComplete Complete(PartialEdge partial) {
      if (!partial.Valid()) 
        return NBestComplete(NULL, lm::ngram::ChartState(), -INFINITY);
      void *place_final = pool_.Allocate(Applied::Size(partial.GetArity()));
      Applied(place_final, partial);
      return NBestComplete(
          place_final,
          partial.CompletedState(),
          partial.GetScore());
    }

  private:
    util::Pool pool_;
};

} // namespace search

#endif // SEARCH_APPLIED__
