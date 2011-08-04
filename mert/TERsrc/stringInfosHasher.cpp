#include "stringInfosHasher.h"
// The following class defines a hash function for strings


using namespace std;

namespace HashMapSpace
{
stringInfosHasher::stringInfosHasher ( long cle, string cleTxt, vector<string> valueVecInt )
{
  m_hashKey=cle;
  m_key=cleTxt;
  m_value=valueVecInt;
}
//     stringInfosHasher::~stringInfosHasher(){};*/
long  stringInfosHasher::getHashKey()
{
  return m_hashKey;
}
string  stringInfosHasher::getKey()
{
  return m_key;
}
vector<string> stringInfosHasher::getValue()
{
  return m_value;
}
void stringInfosHasher::setValue ( vector<string>   value )
{
  m_value=value;
}


// typedef stdext::hash_map<string, string, stringhasher> HASH_S_S;
}
