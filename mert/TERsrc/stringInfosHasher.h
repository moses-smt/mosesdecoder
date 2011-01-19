#ifndef __STRINGINFOSHASHER_H__
#define __STRINGINFOSHASHER_H__
#include <string>
// #include <ext/hash_map>
#include <iostream>
#include <vector>

using namespace std;
namespace HashMapSpace
{
    class stringInfosHasher
    {
        private:
            std::size_t m_hashKey;
            std::string m_key;
            vector<string> m_value;

        public:
            stringInfosHasher ( std::size_t cle, std::string cleTxt, vector<string> valueVecInt );
            std::size_t getHashKey();
            std::string getKey();
            vector<string> getValue();
            void setValue ( vector<string> value );


    };


}
#endif