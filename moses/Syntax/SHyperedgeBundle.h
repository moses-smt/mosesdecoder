#pragma once

#include <vector>

#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhraseCollection.h"

#include "SVertexStack.h"

namespace Moses
{
namespace Syntax
{

struct PVertex;

struct SHyperedgeBundle {
  float inputWeight;
  std::vector<const SVertexStack*> stacks;
  TargetPhraseCollection::shared_ptr translations;

  friend void swap(SHyperedgeBundle &x, SHyperedgeBundle &y) {
    using std::swap;
    swap(x.inputWeight, y.inputWeight);
    swap(x.stacks, y.stacks);
    swap(x.translations, y.translations);
  }
};

}  // Syntax
}  // Moses
