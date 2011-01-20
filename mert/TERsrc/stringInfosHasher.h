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
#endif