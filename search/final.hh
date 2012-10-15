#ifndef SEARCH_FINAL__
#define SEARCH_FINAL__

#include "search/arity.hh"
#include "search/note.hh"
#include "search/types.hh"

#include <boost/array.hpp>

namespace search {

class Final {
  public:
    typedef boost::array<const Final*, search::kMaxArity> ChildArray;

    void Reset(Score bound, Note note, const Final &left, const Final &right) {
      bound_ = bound;
      note_ = note;
      children_[0] = &left;
      children_[1] = &right;
    }

    const ChildArray &Children() const { return children_; }

    Note GetNote() const { return note_; }

    Score Bound() const { return bound_; }

  private:
    Score bound_;

    Note note_;

    ChildArray children_;
};

} // namespace search

#endif // SEARCH_FINAL__
