#pragma once
#include "../PhraseTable.h"
#include "BlockHashIndex.h"

namespace Moses2
{
class PhraseDecoder;
class TPCompact;

class PhraseTableCompact: public PhraseTable
{
public:
  PhraseTableCompact(size_t startInd, const std::string &line);
  virtual ~PhraseTableCompact();
  void Load(System &system);
  virtual void SetParameter(const std::string& key, const std::string& value);

  virtual void CleanUpAfterSentenceProcessing() const;

  virtual TargetPhrases *Lookup(const Manager &mgr, MemPool &pool,
      InputPath &inputPath) const;

  // scfg
  virtual void InitActiveChart(
      MemPool &pool,
      const SCFG::Manager &mgr,
      SCFG::InputPath &path) const;

  virtual void Lookup(const Manager &mgr, InputPathsBase &inputPaths) const;

  virtual void Lookup(
      MemPool &pool,
      const SCFG::Manager &mgr,
      size_t maxChartSpan,
      const SCFG::Stacks &stacks,
      SCFG::InputPath &path) const;

protected:
  static bool s_inMemoryByDefault;
  bool m_inMemory;
  bool m_useAlignmentInfo;

  BlockHashIndex m_hash;

  StringVector<unsigned char, size_t, MmapAllocator>  m_targetPhrasesMapped;
  StringVector<unsigned char, size_t, std::allocator> m_targetPhrasesMemory;

  friend class PhraseDecoder;
  PhraseDecoder* m_phraseDecoder;

  const TargetPhraseImpl *CreateTargetPhrase(
		  const Manager &mgr,
		  const TPCompact &tpCompact,
		  const Phrase<Word> &sourcePhrase) const;

  // SCFG
  virtual void LookupGivenNode(
      MemPool &pool,
      const SCFG::Manager &mgr,
      const SCFG::ActiveChartEntry &prevEntry,
      const SCFG::Word &wordSought,
      const Moses2::Hypotheses *hypos,
      const Moses2::Range &subPhraseRange,
      SCFG::InputPath &outPath) const;

};

}
