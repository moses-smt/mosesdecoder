#pragma once

#include "moses/Word.h"
#include "moses/WordsRange.h"

namespace Moses
{
namespace Syntax
{

struct PVertex {
public:
  PVertex(const WordsRange &wr, const Word &w) : span(wr), symbol(w) {}

  WordsRange span;
  Word symbol;
};

}  // Syntax
}  // Moses
