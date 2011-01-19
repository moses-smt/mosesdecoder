#include "stringHasher.h"
// The following class defines a hash function for strings


using namespace std;

namespace HashMapSpace
{
    stringHasher::stringHasher ( std::size_t cle, std::string cleTxt, std::string valueTxt )
    {
        m_hashKey=cle;
        m_key=cleTxt;
        m_value=valueTxt;
    }
//     stringHasher::~stringHasher(){};*/
    std::size_t  stringHasher::getHashKey()
    {
        return m_hashKey;
    }
    std::string  stringHasher::getKey()
    {
        return m_key;
    }
    std::string  stringHasher::getValue()
    {
        return m_value;
    }
    void stringHasher::setValue ( std::string  value )
    {
        m_value=value;
    }


// typedef stdext::hash_map<std::string, std::string, stringhasher> HASH_S_S;
}
