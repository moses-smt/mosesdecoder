#pragma once

#include "vector"

#include <boost/shared_ptr.hpp>

#include "Symbol.h"

namespace MosesTraining
{
namespace Syntax
{
namespace PostprocessEgretForests
{

class Forest
{
public:
  struct Vertex;

  struct Hyperedge {
    double weight;
    Vertex *head;
    std::vector<Vertex *> tail;
  };

  struct Vertex {
    Symbol symbol;
    int start;
    int end;
    std::vector<boost::shared_ptr<Hyperedge> > incoming;
  };

  Forest() {}

  std::vector<boost::shared_ptr<Vertex> > vertices;

private:
  // Copying is not allowed.
  Forest(const Forest &);
  Forest &operator=(const Forest &);
};

}  // namespace PostprocessEgretForests
}  // namespace Syntax
}  // namespace MosesTraining
