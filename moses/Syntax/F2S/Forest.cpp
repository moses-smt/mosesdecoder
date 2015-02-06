#include "Forest.h"

namespace Moses
{
namespace Syntax
{
namespace F2S
{

Forest::~Forest()
{
  Clear();
}

void Forest::Clear()
{
  for (std::vector<Vertex *>::iterator p = vertices.begin();
       p != vertices.end(); ++p) {
    delete *p;
  }
  vertices.clear();
}

Forest::Vertex::~Vertex()
{
  for (std::vector<Hyperedge *>::iterator p = incoming.begin();
       p != incoming.end(); ++p) {
    delete *p;
  }
}

}  // F2S
}  // Syntax
}  // Moses
