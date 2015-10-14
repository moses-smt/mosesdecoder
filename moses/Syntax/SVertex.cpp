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

size_t SVertex::hash() const
{
  size_t seed;

  // states
  for (size_t i = 0; i < states.size(); ++i) {
	  const FFState *state = states[i];
	  size_t hash = state->hash();
	  boost::hash_combine(seed, hash);
  }
  return seed;

}

bool SVertex::operator==(const SVertex& other) const
{
  // states
  for (size_t i = 0; i < states.size(); ++i) {
	  const FFState &thisState = *states[i];
	  const FFState &otherState = *other.states[i];
	  if (thisState != otherState) {
		  return false;
	  }
  }
  return true;
}

}  // Syntax
}  // Moses
