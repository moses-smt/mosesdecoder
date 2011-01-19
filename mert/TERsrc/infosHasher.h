#ifndef __INFOSHASHER_H__
#define __INFOSHASHER_H__
#include <string>
// #include <ext/hash_map>
#include <stdio.h>
#include <iostream>
#include <vector>

using namespace std;
namespace HashMapSpace
{
    class infosHasher
    {
        private:
            size_t m_hashKey;
            string m_key;
            vector<int> m_value;

        public:
            infosHasher ( size_t cle, string cleTxt, vector<int> valueVecInt );
            size_t getHashKey();
            string getKey();
            vector<int> getValue();
            void setValue ( vector<int> value );


    };


}
#endif