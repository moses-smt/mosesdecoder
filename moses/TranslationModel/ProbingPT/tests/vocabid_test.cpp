#include <map> //Map for vocab ids

#include "hash.hh"
#include "vocabid.hh"

int main(int argc, char* argv[])
{

  //Create a map and serialize it
  std::map<uint64_t, std::string> vocabids;
  StringPiece demotext = StringPiece("Demo text with 3 elements");
  add_to_map(&vocabids, demotext);
  //Serialize map
  serialize_map(&vocabids, "/tmp/testmap.bin");

  //Read the map and test if the values are the same
  std::map<uint64_t, std::string> newmap;
  read_map(&newmap, "/tmp/testmap.bin");

  //Used hashes
  uint64_t num1 = getHash(StringPiece("Demo"));
  uint64_t num2 = getVocabID("text");
  uint64_t num3 = getHash(StringPiece("with"));
  uint64_t num4 = getVocabID("3");
  uint64_t num5 = getHash(StringPiece("elements"));
  uint64_t num6 = 0;

  //Tests
  bool test1 = getStringFromID(&newmap, num1) == getStringFromID(&vocabids, num1);
  bool test2 = getStringFromID(&newmap, num2) == getStringFromID(&vocabids, num2);
  bool test3 = getStringFromID(&newmap, num3) == getStringFromID(&vocabids, num3);
  bool test4 = getStringFromID(&newmap, num4) == getStringFromID(&vocabids, num4);
  bool test5 = getStringFromID(&newmap, num5) == getStringFromID(&vocabids, num5);
  bool test6 = getStringFromID(&newmap, num6) == getStringFromID(&vocabids, num6);


  if (test1 && test2 && test3 && test4 && test5 && test6) {
    std::cout << "Map was successfully written and read!" << std::endl;
  } else {
    std::cout << "Error! " << test1 << " " << test2 << " " << test3 << " " << test4 << " " << test5 << " " << test6 << std::endl;
  }


  return 1;

}
