#ifndef SEARCH_FINAL__
#define SEARCH_FINAL__

#include "search/header.hh"
#include "util/pool.hh"

namespace search {

// A full hypothesis with pointers to children.
class Final : public Header {
  public:
    Final() {}

    Final(util::Pool &pool, Score score, Arity arity, Note note) 
      : Header(pool.Allocate(Size(arity)), arity) {
      SetScore(score);
      SetNote(note);
    }

    // These are arrays of length GetArity().
    Final *Children() {
      return reinterpret_cast<Final*>(After());
    }
    const Final *Children() const {
      return reinterpret_cast<const Final*>(After());
    }

  private:
    static std::size_t Size(Arity arity) {
      return kHeaderSize + arity * sizeof(const Final);
    }
};

} // namespace search

#endif // SEARCH_FINAL__
