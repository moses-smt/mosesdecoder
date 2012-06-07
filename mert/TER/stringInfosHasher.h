#ifndef MERT_TER_STRING_INFOS_HASHER_H_
#define MERT_TER_STRING_INFOS_HASHER_H_

#include <string>
#include <iostream>
#include <vector>

using namespace std;
namespace HashMapSpace
{
class stringInfosHasher
{
private:
  long m_hashKey;
  string m_key;
  vector<string> m_value;

public:
  stringInfosHasher ( long cle, string cleTxt, vector<string> valueVecInt );
  long getHashKey();
  string getKey();
  vector<string> getValue();
  void setValue ( vector<string> value );
};

}

#endif  // MERT_TER_STRING_INFOS_HASHER_H_
