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
            long m_hashKey;
            string m_key;
            vector<int> m_value;

        public:
            infosHasher ( long cle, string cleTxt, vector<int> valueVecInt );
            long getHashKey();
            string getKey();
            vector<int> getValue();
            void setValue ( vector<int> value );


    };


}
#endif