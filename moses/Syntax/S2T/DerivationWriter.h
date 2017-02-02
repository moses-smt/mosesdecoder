#pragma once

#include <ostream>

#include "moses/Syntax/KBestExtractor.h"
#include "moses/Word.h"

namespace Moses
{
namespace Syntax
{
struct SHyperedge;

namespace S2T
{

// Writes a string representation of a derivation to a std::ostream.  This is
// used by the -translation-details / -T option.
// TODO DerivationWriter currently assumes string-to-tree (which is why it's
// TODO in the S2T namespace) but it would be easy to generalise it.  This
// TODO should be revisited when other the decoders are implemented.
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

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses
