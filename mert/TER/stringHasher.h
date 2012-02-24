#ifndef MERT_TER_STRING_HASHER_H_
#define MERT_TER_STRING_HASHER_H_

#include <string>
#include <iostream>

using namespace std;
namespace HashMapSpace
{

class stringHasher
{
private:
  long m_hashKey;
  string m_key;
  string m_value;

public:
  stringHasher ( long cle, string cleTxt, string valueTxt );
  long getHashKey();
  string getKey();
  string getValue();
  void setValue ( string value );
};

}

#endif  // MERT_TER_STRING_HASHER_H_
