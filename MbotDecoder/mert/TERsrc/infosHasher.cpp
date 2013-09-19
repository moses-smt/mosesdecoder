#include "infosHasher.h"
// The following class defines a hash function for strings


using namespace std;

namespace HashMapSpace
{
infosHasher::infosHasher (long cle,string cleTxt, vector<int> valueVecInt )
{
  m_hashKey=cle;
  m_key=cleTxt;
  m_value=valueVecInt;
}
//     infosHasher::~infosHasher(){};*/
long  infosHasher::getHashKey()
{
  return m_hashKey;
}
string  infosHasher::getKey()
{
  return m_key;
}
vector<int> infosHasher::getValue()
{
  return m_value;
}
void infosHasher::setValue ( vector<int>   value )
{
  m_value=value;
}


// typedef stdext::hash_map<std::string,string, stringhasher> HASH_S_S;
}
