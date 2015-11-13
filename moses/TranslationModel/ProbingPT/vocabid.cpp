#include "vocabid.hh"

void add_to_map(std::map<uint64_t, std::string> *karta, StringPiece textin)
{
  //Tokenize
  util::TokenIter<util::SingleCharacter> it(textin, util::SingleCharacter(' '));

  while(it) {
    karta->insert(std::pair<uint64_t, std::string>(getHash(*it), it->as_string()));
    it++;
  }
}

void serialize_map(std::map<uint64_t, std::string> *karta, const char* filename)
{
  std::ofstream os (filename, std::ios::binary);
  boost::archive::text_oarchive oarch(os);

  oarch << *karta;  //Serialise map
  os.close();
}

void read_map(std::map<uint64_t, std::string> *karta, const char* filename)
{
  std::ifstream is (filename, std::ios::binary);
  boost::archive::text_iarchive iarch(is);

  iarch >> *karta;

  //Close the stream after we are done.
  is.close();
}
