/*
 * Generic hashmap manipulation functions
 */
#ifndef __HASHMAPSTRINGINFOS_H__
#define __HASHMAPSTRINGINFOS_H__
#include <boost/functional/hash.hpp>
#include "stringInfosHasher.h"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

using namespace std;

namespace HashMapSpace
{
    class hashMapStringInfos
    {
        private:
            vector<stringInfosHasher> m_hasher;

        public:
//     ~hashMap();
            long hashValue ( string key );
            int trouve ( long searchKey );
            int trouve ( string key );
            void addHasher ( string key, vector<string>  value );
            void addValue ( string key, vector<string>  value );
            stringInfosHasher getHasher ( string key );
            vector<string> getValue ( string key );
//         string searchValue ( string key );
            void setValue ( string key , vector<string>  value );
            void printHash();
            vector<stringInfosHasher> getHashMap();
            string printStringHash();
            string printStringHash2();
            string printStringHashForLexicon();
    };


}


#endif
