#include <boost/foreach.hpp>
#include "vocabid.h"
#include "StoreVocab.h"
#include "moses2/legacy/Util2.h"

namespace probingpt
{

void add_to_map(StoreVocab<uint64_t> &sourceVocab,
                const StringPiece &textin)
{
  //Tokenize
  util::TokenIter<util::SingleCharacter> itWord(textin, util::SingleCharacter(' '));

  while (itWord) {
    StringPiece word = *itWord;

    util::TokenIter<util::SingleCharacter> itFactor(word, util::SingleCharacter('|'));
    while (itFactor) {
      StringPiece factor = *itFactor;

      sourceVocab.Insert(getHash(factor), factor.as_string());
      itFactor++;
    }
    itWord++;
  }
}

void serialize_map(const std::map<uint64_t, std::string> &karta,
                   const std::string &filename)
{
  std::ofstream os(filename.c_str());

  std::map<uint64_t, std::string>::const_iterator iter;
  for (iter = karta.begin(); iter != karta.end(); ++iter) {
    os << iter->first << '\t' << iter->second << std::endl;
  }

  os.close();
}

void read_map(std::map<uint64_t, std::string> &karta, const char* filename)
{
  std::ifstream is(filename);

  std::string line;
  while (getline(is, line)) {
    std::vector<std::string> toks = Moses2::Tokenize(line, "\t");
    assert(toks.size() == 2);
    uint64_t ind = Moses2::Scan<uint64_t>(toks[1]);
    karta[ind] = toks[0];
  }

  //Close the stream after we are done.
  is.close();
}

}

