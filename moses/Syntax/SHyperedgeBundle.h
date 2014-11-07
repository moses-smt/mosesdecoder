#pragma once

#include <vector>

#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhraseCollection.h"

#include "SVertexBeam.h"

namespace Moses
{
namespace Syntax
{

struct PVertex;

struct SHyperedgeBundle
{
  std::vector<const SVertexBeam*> beams;
  const TargetPhraseCollection *translations;

  friend void swap(SHyperedgeBundle &x, SHyperedgeBundle &y) {
    using std::swap;
    swap(x.beams, y.beams);
    swap(x.translations, y.translations);
  }
};

}  // Syntax
}  // Moses
