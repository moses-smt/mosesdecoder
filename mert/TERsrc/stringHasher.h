#ifndef __STRINGHASHER_H__
#define __STRINGHASHER_H__
#include <string>
//#include <ext/hash_map>
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
#endif
