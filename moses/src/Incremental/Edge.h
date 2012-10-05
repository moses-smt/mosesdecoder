#pragma once

#include "search/edge.hh"

namespace Moses {
namespace Incremental {

class Edge : public search::Edge {
  public:
    explicit Edge(const TargetPhrase &origin) : origin_(origin) {}

  private:
    const TargetPhrase &origin_;
};

} // namespace Incremental
} // namespace Moses
