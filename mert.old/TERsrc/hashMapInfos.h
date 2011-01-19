/*
 * Generic hashmap manipulation functions
 */
#ifndef __HASHMAPINFOS_H__
#define __HASHMAPINFOS_H__
#include <boost/functional/hash.hpp>
#include "infosHasher.h"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

using namespace std;

namespace HashMapSpace
{
    class hashMapInfos
    {
        private:
            std::vector<infosHasher> m_hasher;

        public:
//     ~hashMap();
            std::size_t hashValue ( std::string key );
            int trouve ( std::size_t searchKey );
            int trouve ( std::string key );
            void addHasher ( std::string key, vector<int>  value );
            void addValue ( std::string key, vector<int>  value );
            infosHasher getHasher ( std::string key );
            vector<int> getValue ( std::string key );
//         std::string searchValue ( std::string key );
            void setValue ( std::string key , vector<int>  value );
            void printHash();
            std::vector<infosHasher> getHashMap();
            string printStringHash();
            string printStringHash2();
            string printStringHashForLexicon();
    };


}


#endif
