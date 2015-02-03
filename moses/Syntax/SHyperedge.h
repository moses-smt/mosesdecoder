#pragma once

#include <vector>

#include "moses/Phrase.h"

#include "SLabel.h"

namespace Moses
{
namespace Syntax
{

struct SVertex;

struct SHyperedge {
  SVertex *head;
  std::vector<SVertex*> tail;
  SLabel label;
};

Phrase GetOneBestTargetYield(const SHyperedge &h);

}  // Syntax
}  // Moses
