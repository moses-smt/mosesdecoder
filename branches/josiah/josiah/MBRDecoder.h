#pragma once

#include <vector>
#include <utility>
#include <ext/hash_map>

#include "Gibbler.h"
#include "ScoreComponentCollection.h"
#include "GibblerMaxTransDecoder.h"
#include "GainFunction.h"
#include "Phrase.h"

using namespace Moses;

namespace Josiah {

class MBRDecoder : public GibblerMaxTransDecoder {
 public:
   MBRDecoder(int size) : MaxCollector<Translation>("MBR"),  mbrSize(size) {};
  virtual pair<const Translation*,float> getMax();
  void SetMBRSize(int size) { mbrSize = size;}

 private:
  GainFunctionVector g;
  int mbrSize;
};

}


