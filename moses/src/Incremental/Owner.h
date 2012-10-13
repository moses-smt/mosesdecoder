#pragma once

#include "Edge.h"
#include "search/vertex.hh"

#include <boost/pool/object_pool.hpp>

namespace Moses {
namespace Incremental {

class Owner {
  public:
    boost::object_pool<search::Vertex> &VertexPool() { return vertices_; }

  private:
    boost::object_pool<search::Vertex> vertices_;
};

} // namespace Incremental
} // namespace Moses
