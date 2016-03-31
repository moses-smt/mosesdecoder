/*
 * SearchNormal.h
 *
 *  Created on: 25 Oct 2015
 *      Author: hieu
 */

#ifndef SEARCHNORMAL_H_
#define SEARCHNORMAL_H_
#include <vector>
#include "../../legacy/Range.h"
#include "../../legacy/Bitmap.h"
#include "../../TypeDef.h"
#include "../Search.h"
#include "Stacks.h"

namespace Moses2
{

class Hypothesis;
class InputPath;
class TargetPhrases;
class TargetPhrase;
class Stacks;

class SearchNormal: public Search
{
public:
  SearchNormal(Manager &mgr);
  virtual ~SearchNormal();

  virtual void Decode();
  const Hypothesis *GetBestHypothesis() const;

  void AddInitialTrellisPaths(TrellisPaths &paths) const;

protected:
  Stacks m_stacks;

  void Decode(size_t stackInd);
  void Extend(const Hypothesis &hypo, const InputPath &path);
  void Extend(const Hypothesis &hypo, const TargetPhrases &tps,
      const InputPath &path, const Bitmap &newBitmap, SCORE estimatedScore);
  void Extend(const Hypothesis &hypo, const TargetPhrase &tp,
      const InputPath &path, const Bitmap &newBitmap, SCORE estimatedScore);

};

}

#endif /* SEARCHNORMAL_H_ */
