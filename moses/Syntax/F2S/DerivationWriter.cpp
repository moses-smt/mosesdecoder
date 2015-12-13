#include "DerivationWriter.h"

#include "moses/Factor.h"
#include "moses/Syntax/PVertex.h"
#include "moses/Syntax/SHyperedge.h"

namespace Moses
{
namespace Syntax
{
namespace F2S
{

// 1-best version.
void DerivationWriter::Write(const SHyperedge &shyperedge,
                             std::size_t sentNum, std::ostream &out)
{
  WriteLine(shyperedge, sentNum, out);
  for (std::size_t i = 0; i < shyperedge.tail.size(); ++i) {
    const SVertex &pred = *(shyperedge.tail[i]);
    if (pred.best) {
      Write(*pred.best, sentNum, out);
    }
  }
}

// k-best derivation.
void DerivationWriter::Write(const KBestExtractor::Derivation &derivation,
                             std::size_t sentNum, std::ostream &out)
{
  WriteLine(derivation.edge->shyperedge, sentNum, out);
  for (std::size_t i = 0; i < derivation.subderivations.size(); ++i) {
    Write(*(derivation.subderivations[i]), sentNum, out);
  }
}

void DerivationWriter::WriteLine(const SHyperedge &shyperedge,
                                 std::size_t sentNum, std::ostream &out)
{
  // Sentence number.
  out << sentNum << " |||";

  // Source LHS.
  out << " ";
  WriteSymbol(shyperedge.head->pvertex->symbol, out);
  out << " ->";

  // Source RHS symbols.
  for (std::size_t i = 0; i < shyperedge.tail.size(); ++i) {
    const Word &symbol = shyperedge.tail[i]->pvertex->symbol;
    out << " ";
    WriteSymbol(symbol, out);
  }
  out << " |||";

  // Target RHS.
  out << " [X] ->";

  // Target RHS symbols.
  const TargetPhrase &phrase = *(shyperedge.label.translation);
  for (std::size_t i = 0; i < phrase.GetSize(); ++i) {
    const Word &symbol = phrase.GetWord(i);
    out << " ";
    if (symbol.IsNonTerminal()) {
      out << "[X]";
    } else {
      WriteSymbol(symbol, out);
    }
  }
  out << " |||";

  // Non-terminal alignments
  const AlignmentInfo &a = phrase.GetAlignNonTerm();
  for (AlignmentInfo::const_iterator p = a.begin(); p != a.end(); ++p) {
    out << " " << p->first << "-" << p->second;
  }
  out << " |||";

  // Spans covered by source RHS symbols.
  for (std::size_t i = 0; i < shyperedge.tail.size(); ++i) {
    const SVertex *child = shyperedge.tail[i];
    const Range &span = child->pvertex->span;
    out << " " << span.GetStartPos() << ".." << span.GetEndPos();
  }

  out << "\n";
}

void DerivationWriter::WriteSymbol(const Word &symbol, std::ostream &out)
{
  const Factor *f = symbol[0];
  if (symbol.IsNonTerminal()) {
    out << "[" << f->GetString() << "]";
  } else {
    out << f->GetString();
  }
}

}  // namespace F2S
}  // namespace Syntax
}  // namespace Moses
