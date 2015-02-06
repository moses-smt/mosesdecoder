#pragma once

#include <vector>

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

template<typename T>
struct Forest {
  struct Vertex;

  struct Hyperedge {
    Vertex *head;
    std::vector<Vertex *> tail;
  };

  struct Vertex {
    ~Vertex();
    T value;
    std::vector<Hyperedge *> incoming;
  };

  Forest() {}

  ~Forest();

  std::vector<Vertex *> vertices;

 private:
  // Copying is not allowed.
  Forest(const Forest &);
  Forest &operator=(const Forest &);
};

template<typename T>
Forest<T>::~Forest()
{
  for (typename std::vector<Vertex *>::iterator p = vertices.begin();
       p != vertices.end(); ++p) {
    delete *p;
  }
}

template<typename T>
Forest<T>::Vertex::~Vertex()
{
  for (typename std::vector<Hyperedge *>::iterator p = incoming.begin();
       p != incoming.end(); ++p) {
    delete *p;
  }
}

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace Moses
