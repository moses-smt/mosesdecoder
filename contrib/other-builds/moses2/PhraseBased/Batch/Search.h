/*
 * SearchNormal.h
 *
 *  Created on: 25 Oct 2015
 *      Author: hieu
 */
#pragma once

#include <vector>
#include "../../legacy/Range.h"
#include "../../legacy/Bitmap.h"
#include "../../TypeDef.h"
#include "../Search.h"
#include "Stacks.h"

namespace Moses2
{
template<typename WORD>
class TargetPhrase;

class Hypothesis;
class InputPath;
class TargetPhrases;

namespace NSBatch
{
class Stacks;

class Search: public Moses2::Search
{
public:
  Search(Manager &mgr);
  virtual ~Search();

  virtual void Decode();
  const Hypothesis *GetBestHypothesis() const;

  void AddInitialTrellisPaths(TrellisPaths &paths) const;

protected:
  Stacks m_stacks;

  Batch &m_batch;

  void Decode(size_t stackInd);
  void Extend(const Hypothesis &hypo, const InputPath &path);
  void Extend(const Hypothesis &hypo, const TargetPhrases &tps,
      const InputPath &path, const Bitmap &newBitmap, SCORE estimatedScore);
  void Extend(const Hypothesis &hypo, const TargetPhrase<Moses2::Word> &tp,
      const InputPath &path, const Bitmap &newBitmap, SCORE estimatedScore);

};

}
}
