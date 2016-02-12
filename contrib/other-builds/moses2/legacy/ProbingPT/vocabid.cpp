#include <boost/foreach.hpp>
#include "vocabid.hh"

namespace Moses2
{

void add_to_map(std::map<uint64_t, std::string> *karta, const StringPiece &textin)
{
  //Tokenize
  util::TokenIter<util::SingleCharacter> it(textin, util::SingleCharacter(' '));

  while(it) {
    karta->insert(std::pair<uint64_t, std::string>(getHash(*it), it->as_string()));
    it++;
  }
}

void serialize_map(const std::map<uint64_t, std::string> &karta, const std::string &filename)
{
	  std::ofstream os (filename.c_str());

	  std::map<uint64_t, std::string>::const_iterator iter;
	  for (iter = karta.begin(); iter != karta.end(); ++iter) {
		  os << iter->first << '\t' << iter->second << std::endl;
	  }

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

}


