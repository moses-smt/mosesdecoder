#pragma once

#include <vector>

#include "PLabel.h"

namespace Moses
{
namespace Syntax
{

struct PVertex;

struct PHyperedge {
  PVertex *head;
  std::vector<PVertex*> tail;
  PLabel label;
};

}  // Syntax
}  // Moses
