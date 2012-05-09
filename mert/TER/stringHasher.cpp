#include "stringHasher.h"
// The following class defines a hash function for strings


using namespace std;

namespace HashMapSpace
{
stringHasher::stringHasher ( long cle, string cleTxt, string valueTxt )
{
  m_hashKey=cle;
  m_key=cleTxt;
  m_value=valueTxt;
}
//     stringHasher::~stringHasher(){};*/
long  stringHasher::getHashKey()
{
  return m_hashKey;
}
string  stringHasher::getKey()
{
  return m_key;
}
string  stringHasher::getValue()
{
  return m_value;
}
void stringHasher::setValue ( string  value )
{
  m_value=value;
}


// typedef stdext::hash_map<string, string, stringhasher> HASH_S_S;
}
