#pragma once

#include "moses/TargetPhraseCollection.h"

namespace Moses
{
namespace Syntax
{

struct PLabel {
  const TargetPhraseCollection *translations;
};

}  // Syntax
}  // Moses
