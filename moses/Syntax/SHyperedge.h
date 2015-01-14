#pragma once

#include <vector>

#include "moses/Phrase.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"

namespace Moses
{
namespace Syntax
{

struct SVertex;

struct SHyperedge {
  SVertex *head;
  std::vector<SVertex*> tail;
  float score;
  ScoreComponentCollection scoreBreakdown;
  const TargetPhrase *translation;
};

Phrase GetOneBestTargetYield(const SHyperedge &h);

}  // Syntax
}  // Moses
