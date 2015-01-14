#pragma once

#include <vector>

#include "moses/TargetPhraseCollection.h"

namespace Moses
{
namespace Syntax
{

struct PVertex;

struct PHyperedge {
  PVertex *head;
  std::vector<PVertex*> tail;
  const TargetPhraseCollection *translations;
};

}  // Syntax
}  // Moses
