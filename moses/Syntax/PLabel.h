#pragma once

#include "moses/TargetPhraseCollection.h"

namespace Moses
{
namespace Syntax
{

struct PLabel {
  float inputWeight;
  const TargetPhraseCollection *translations;
};

}  // Syntax
}  // Moses
