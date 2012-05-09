/*
 * Generic hashmap manipulation functions
 */
#ifndef MERT_TER_HASHMAP_INFOS_H_
#define MERT_TER_HASHMAP_INFOS_H_

#include "infosHasher.h"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

using namespace std;

namespace HashMapSpace
{
class hashMapInfos
{
private:
  vector<infosHasher> m_hasher;

public:
//     ~hashMap();
  long hashValue ( string key );
  int trouve ( long searchKey );
  int trouve ( string key );
  void addHasher ( string key, vector<int>  value );
  void addValue ( string key, vector<int>  value );
  infosHasher getHasher ( string key );
  vector<int> getValue ( string key );
//         string searchValue ( string key );
  void setValue ( string key , vector<int>  value );
  void printHash();
  vector<infosHasher> getHashMap();
  string printStringHash();
  string printStringHash2();
  string printStringHashForLexicon();
};

}

#endif  // MERT_TER_HASHMAP_INFOS_H_
