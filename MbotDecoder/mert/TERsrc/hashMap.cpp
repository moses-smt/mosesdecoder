#include "hashMap.h"

// The following class defines a hash function for strings


using namespace std;

namespace HashMapSpace
{
//     hashMap::hashMap();
/*    hashMap::~hashMap()
    {
//       vector<stringHasher>::const_iterator del = m_hasher.begin();
      for ( vector<stringHasher>::const_iterator del=m_hasher.begin(); del != m_hasher.end(); del++ )
      {
        delete(*del);
      }
    }*/
/**
 * int hashMap::trouve ( long searchKey )
 * @param searchKey
 * @return
 */
int hashMap::trouve ( long searchKey )
{
  long foundKey;
//       vector<stringHasher>::const_iterator l_hasher=m_hasher.begin();
  for ( vector<stringHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ ) {
    foundKey= ( *l_hasher ).getHashKey();
    if ( searchKey == foundKey ) {
      return 1;
    }
  }
  return 0;
}
int hashMap::trouve ( string key )
{
  long searchKey=hashValue ( key );
  long foundKey;;
//       vector<stringHasher>::const_iterator l_hasher=m_hasher.begin();
  for ( vector<stringHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ ) {
    foundKey= ( *l_hasher ).getHashKey();
    if ( searchKey == foundKey ) {
      return 1;
    }
  }
  return 0;
}
/**
 * long hashMap::hashValue ( string key )
 * @param key
 * @return
 */
long hashMap::hashValue ( string key )
{
  locale loc;                 // the "C" locale
  const collate<char>& coll = use_facet<collate<char> >(loc);
  return coll.hash(key.data(),key.data()+key.length());
//         boost::hash<string> hasher;
//         return hasher ( key );
}
/**
 * void hashMap::addHasher ( string key, string value )
 * @param key
 * @param value
 */
void hashMap::addHasher ( string key, string value )
{
  if ( trouve ( hashValue ( key ) ) ==0 ) {
//         cerr << "ICI1" <<endl;
    stringHasher H ( hashValue ( key ),key,value );
//         cerr <<" "<< hashValue ( key )<<" "<< key<<" "<<value <<endl;
//         cerr << "ICI2" <<endl;

    m_hasher.push_back ( H );
  }
}
stringHasher hashMap::getHasher ( string key )
{
  long searchKey=hashValue ( key );
  long foundKey;
  stringHasher defaut(0,"","");
//       vector<stringHasher>::const_iterator l_hasher=m_hasher.begin();
  for ( vector<stringHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ ) {
    foundKey= ( *l_hasher ).getHashKey();
    if ( searchKey == foundKey ) {
      return ( *l_hasher );
    }
  }
  return defaut;
}
string hashMap::getValue ( string key )
{
  long searchKey=hashValue ( key );
  long foundKey;
//       vector<stringHasher>::const_iterator l_hasher=m_hasher.begin();
  for ( vector<stringHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ ) {
    foundKey= ( *l_hasher ).getHashKey();
    if ( searchKey == foundKey ) {
//         cerr <<"value found : " << key<<"|"<< ( *l_hasher ).getValue()<<endl;
      return ( *l_hasher ).getValue();
    }
  }
  return "";
}
string hashMap::searchValue ( string value )
{
//       long searchKey=hashValue ( key );
//       long foundKey;
  string foundValue;

//       vector<stringHasher>::const_iterator l_hasher=m_hasher.begin();
  for ( vector<stringHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ ) {
    foundValue= ( *l_hasher ).getValue();
    if ( foundValue.compare ( value ) == 0 ) {
      return ( *l_hasher ).getKey();
    }
  }
  return "";
}


void hashMap::setValue ( string key , string value )
{
  long searchKey=hashValue ( key );
  long foundKey;
//       vector<stringHasher>::const_iterator l_hasher=m_hasher.begin();
  for ( vector<stringHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ ) {
    foundKey= ( *l_hasher ).getHashKey();
    if ( searchKey == foundKey ) {
      ( *l_hasher ).setValue ( value );
//           return ( *l_hasher ).getValue();
    }
  }
}


/**
 *
 */
void hashMap::printHash()
{
  for ( vector<stringHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ ) {
    cout << ( *l_hasher ).getHashKey() <<" | "<< ( *l_hasher ).getKey() << " | " << ( *l_hasher ).getValue() << endl;
  }
}



//     long hashValue(string key){}

}

