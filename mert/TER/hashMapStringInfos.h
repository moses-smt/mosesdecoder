/*
 * Generic hashmap manipulation functions
 */
#ifndef MERT_TER_HASHMAP_STRING_INFOS_H_
#define MERT_TER_HASHMAP_STRING_INFOS_H_

#include "stringInfosHasher.h"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

using namespace std;

namespace HashMapSpace
{
class hashMapStringInfos
{
private:
  vector<stringInfosHasher> m_hasher;

public:
//     ~hashMap();
  long hashValue ( string key );
  int trouve ( long searchKey );
  int trouve ( string key );
  void addHasher ( string key, vector<string>  value );
  void addValue ( string key, vector<string>  value );
  stringInfosHasher getHasher ( string key );
  vector<string> getValue ( string key );
//         string searchValue ( string key );
  void setValue ( string key , vector<string>  value );
  void printHash();
  vector<stringInfosHasher> getHashMap();
  string printStringHash();
  string printStringHash2();
  string printStringHashForLexicon();
};

}

#endif  // MERT_TER_HASHMAP_STRING_INFOS_H_
