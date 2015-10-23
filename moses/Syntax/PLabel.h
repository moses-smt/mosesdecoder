#pragma once

#include "moses/TargetPhraseCollection.h"

namespace Moses
{
namespace Syntax
{

struct PLabel {
  float inputWeight;
  TargetPhraseCollection::shared_ptr translations;
};

}  // Syntax
}  // Moses
