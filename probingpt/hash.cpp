#include <iostream>
#include "hash.h"

using namespace std;

namespace probingpt
{

uint64_t getHash(StringPiece text)
{
  std::size_t len = text.size();
  uint64_t key = util::MurmurHashNative(text.data(), len);
  return key;
}

std::vector<uint64_t> getVocabIDs(const StringPiece &textin)
{
  //Tokenize
  std::vector<uint64_t> output;

  util::TokenIter<util::SingleCharacter> itWord(textin, util::SingleCharacter(' '));

  while (itWord) {
    StringPiece word = *itWord;
    uint64_t id = 0;

    util::TokenIter<util::SingleCharacter> itFactor(word, util::SingleCharacter('|'));
    while (itFactor) {
      StringPiece factor = *itFactor;
      //cerr << "factor=" << factor << endl;

      id += getHash(factor);
      itFactor++;
    }

    output.push_back(id);
    itWord++;
  }

  return output;
}

}

