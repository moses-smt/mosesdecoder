#ifndef SEARCH_FINAL__
#define SEARCH_FINAL__

#include "search/arity.hh"
#include "search/note.hh"
#include "search/types.hh"

#include <boost/array.hpp>

namespace search {

class Final {
  public:
    const Final **Reset(Score bound, Note note) {
      bound_ = bound;
      note_ = note;
      return children_;
    }

    const Final *const *Children() const { return children_; }

    Note GetNote() const { return note_; }

    Score Bound() const { return bound_; }

  private:
    Score bound_;

    Note note_;

    const Final *children_[2];
};

} // namespace search

#endif // SEARCH_FINAL__
