#include <boost/foreach.hpp>
#include "vocabid.hh"
#include "StoreVocab.h"
#include "../Util2.h"

namespace Moses2
{

void add_to_map(StoreVocab<uint64_t> &sourceVocab, const std::string &textin)
{
  //Tokenize
  std::vector<std::string> toks = Tokenize(textin, " ");
  for (size_t i = 0; i < toks.size(); ++i) {
    uint64_t id = sourceVocab.GetVocabId(toks[i]);
  }
}

}

