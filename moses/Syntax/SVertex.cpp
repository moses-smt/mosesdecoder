#include "SVertex.h"

#include "moses/FF/FFState.h"

#include "SHyperedge.h"

namespace Moses
{
namespace Syntax
{

SVertex::~SVertex()
{
  // Delete incoming SHyperedge objects.
  delete best;
  for (std::vector<SHyperedge*>::iterator p = recombined.begin();
       p != recombined.end(); ++p) {
    delete *p;
  }
  // Delete FFState objects.
  for (std::vector<FFState*>::iterator p = states.begin();
       p != states.end(); ++p) {
    delete *p;
  }
}

}  // Syntax
}  // Moses
