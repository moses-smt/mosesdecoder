#pragma once

#include "search/edge.hh"

namespace Moses {

class TargetPhrase;

namespace Incremental {

class Edge : public search::Edge {
  public:
    explicit Edge(const TargetPhrase &origin) : origin_(origin) {}

    const TargetPhrase &GetMoses() const { return origin_; }

  private:
    const TargetPhrase &origin_;
};

} // namespace Incremental
} // namespace Moses
