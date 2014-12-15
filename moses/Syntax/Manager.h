#pragma once

#include "moses/InputType.h"
#include "moses/BaseManager.h"

#include "KBestExtractor.h"

namespace Moses
{
namespace Syntax
{

// Common base class for Moses::Syntax managers.
class Manager : public BaseManager
{
 public:
  Manager(const InputType &);

  // Virtual functions from Moses::BaseManager that are implemented the same
  // way for all Syntax managers.
  void OutputNBest(OutputCollector *collector) const;
  void OutputUnknowns(OutputCollector *collector) const;

  // Virtual functions from Moses::BaseManager that are no-ops for all Syntax
  // managers.
  void OutputLatticeSamples(OutputCollector *collector) const {}
  void OutputAlignment(OutputCollector *collector) const {}
  void OutputDetailedTreeFragmentsTranslationReport(
      OutputCollector *collector) const {}
  void OutputWordGraph(OutputCollector *collector) const {}
  void OutputSearchGraph(OutputCollector *collector) const {}
  void OutputSearchGraphSLF() const {}
  void OutputSearchGraphHypergraph() const {}

  // Syntax-specific virtual functions that derived classes must implement.
  virtual void ExtractKBest(
      std::size_t k,
      std::vector<boost::shared_ptr<KBestExtractor::Derivation> > &kBestList,
      bool onlyDistinct=false) const = 0;

 protected:
  std::set<Word> m_oovs;

 private:
  // Syntax-specific helper functions used to implement OutputNBest.
  void OutputNBestList(OutputCollector *collector,
                       const KBestExtractor::KBestVec &nBestList,
                       long translationId) const;

  std::size_t OutputAlignmentNBest(Alignments &retAlign,
                                   const KBestExtractor::Derivation &d,
                                   std::size_t startTarget) const;

  std::size_t CalcSourceSize(const KBestExtractor::Derivation &d) const;
};

}  // Syntax
}  // Moses
