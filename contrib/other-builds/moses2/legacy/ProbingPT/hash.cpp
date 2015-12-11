#include "hash.hh"

namespace Moses2
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

  util::TokenIter<util::SingleCharacter> it(textin, util::SingleCharacter(' '));

  while(it) {
    output.push_back(getHash(*it));
    it++;
  }

  return output;
}

uint64_t getVocabID(const std::string &candidate)
{
  std::size_t len = candidate.length();
  uint64_t key = util::MurmurHashNative(candidate.c_str(), len);
  return key;
}


}


