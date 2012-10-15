#ifndef SEARCH_SOURCE__
#define SEARCH_SOURCE__

#include "search/types.hh"

#include <assert.h>
#include <vector>

namespace search {

template <class Final> class Source {
  public:
    Source() : bound_(kScoreInf) {}

    Index Size() const {
      return final_.size();
    }

    Score Bound() const {
      return bound_;
    }

    const Final &operator[](Index index) const {
      return *final_[index];
    }

    Score ScoreOrBound(Index index) const {
      return Size() > index ? final_[index]->Total() : Bound();
    }

  protected:
    void AddFinal(const Final &store) {
      final_.push_back(&store);
    }

    void SetBound(Score to) {
      assert(to <= bound_ + 0.001);
      bound_ = to;
    }

  private:
    std::vector<const Final *> final_;

    Score bound_;
};

} // namespace search
#endif // SEARCH_SOURCE__
