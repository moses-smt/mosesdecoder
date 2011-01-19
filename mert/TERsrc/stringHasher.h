#ifndef __STRINGHASHER_H__
#define __STRINGHASHER_H__
#include <string>
//#include <ext/hash_map>
#include <iostream>


namespace HashMapSpace
{

    class stringHasher
    {
        private:
            std::size_t m_hashKey;
            std::string m_key;
            std::string m_value;

        public:
            stringHasher ( std::size_t cle, std::string cleTxt, std::string valueTxt );
            std::size_t getHashKey();
            std::string getKey();
            std::string getValue();
            void setValue ( std::string value );


    };


}
#endif
