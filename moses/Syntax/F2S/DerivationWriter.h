#pragma once

#include <ostream>

#include "moses/Syntax/KBestExtractor.h"
#include "moses/Word.h"

namespace Moses
{
namespace Syntax
{
struct SHyperedge;

namespace F2S
{

// Writes a string representation of a derivation to a std::ostream.  This is
// used by the -translation-details / -T option.
// TODO Merge this with S2T::DerivationWriter.
class DerivationWriter
{
 public:
  // 1-best version.
  static void Write(const SHyperedge&, std::size_t, std::ostream &);

  // k-best version.
  static void Write(const KBestExtractor::Derivation &, std::size_t,
                    std::ostream &);
 private:
  static void WriteLine(const SHyperedge &, std::size_t, std::ostream &);
  static void WriteSymbol(const Word &, std::ostream &);
};

}  // namespace F2S
}  // namespace Syntax
}  // namespace Moses
