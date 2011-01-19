/*
 * Generic hashmap manipulation functions
 */
#ifndef __HASHMAP_H__
#define __HASHMAP_H__
#include <boost/functional/hash.hpp>
#include "stringHasher.h"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

using namespace std;

namespace HashMapSpace
{
    class hashMap
    {
        private:
            std::vector<stringHasher> m_hasher;

        public:
//     ~hashMap();
            std::size_t hashValue ( std::string key );
            int trouve ( std::size_t searchKey );
            int trouve ( string key );
            void addHasher ( std::string key, std::string value );
            stringHasher getHasher ( std::string key );
            std::string getValue ( std::string key );
            std::string searchValue ( std::string key );
            void setValue ( std::string key , std::string value );
            void printHash();
            std::vector<stringHasher> getHashMap();
            string printStringHash();
            string printStringHash2();
            string printStringHashForLexicon();
    };


}


#endif
