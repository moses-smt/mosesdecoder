/*
 * Generic hashmap manipulation functions
 */

#ifndef MERT_TER_HASHMAP_H_
#define MERT_TER_HASHMAP_H_

#include "stringHasher.h"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <locale>

using namespace std;

namespace HashMapSpace
{
class hashMap
{
private:
  vector<stringHasher> m_hasher;

public:
//     ~hashMap();
  long hashValue ( string key );
  int trouve ( long searchKey );
  int trouve ( string key );
  void addHasher ( string key, string value );
  stringHasher getHasher ( string key );
  string getValue ( string key );
  string searchValue ( string key );
  void setValue ( string key , string value );
  void printHash();
  vector<stringHasher> getHashMap();
  string printStringHash();
  string printStringHash2();
  string printStringHashForLexicon();
};

}

#endif  // MERT_TER_HASHMAP_H_
