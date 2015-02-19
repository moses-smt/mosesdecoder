#pragma once

#include "vector"

#include "moses/Syntax/PVertex.h"

namespace Moses
{
namespace Syntax
{
namespace F2S
{

class Forest
{
public:
  struct Vertex;

  struct Hyperedge {
    Vertex *head;
    std::vector<Vertex *> tail;
    float weight;
  };

  struct Vertex {
    Vertex(const PVertex &v) : pvertex(v) {}
    ~Vertex();  // Deletes incoming hyperedges.
    PVertex pvertex;
    std::vector<Hyperedge *> incoming;
  };

  // Constructor.
  Forest() {}

  // Destructor (deletes vertices).
  ~Forest();

  // Delete all vertices.
  void Clear();

  std::vector<Vertex *> vertices;

private:
  // Copying is not allowed.
  Forest(const Forest &);
  Forest &operator=(const Forest &);
};

}  // namespace F2S
}  // namespace Syntax
}  // namespace Moses
