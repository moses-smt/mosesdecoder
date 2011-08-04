#include "hashMapInfos.h"

// The following class defines a hash function for strings


using namespace std;

namespace HashMapSpace
{
//     hashMapInfos::hashMap();
/*    hashMapInfos::~hashMap()
    {
//       vector<infosHasher>::const_iterator del = m_hasher.begin();
      for ( vector<infosHasher>::const_iterator del=m_hasher.begin(); del != m_hasher.end(); del++ )
      {
        delete(*del);
      }
    }*/
/**
 * int hashMapInfos::trouve ( long searchKey )
 * @param searchKey
 * @return
 */
int hashMapInfos::trouve ( long searchKey )
{
  long foundKey;
//       vector<infosHasher>::const_iterator l_hasher=m_hasher.begin();
  for ( vector<infosHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ ) {
    foundKey= ( *l_hasher ).getHashKey();
    if ( searchKey == foundKey ) {
      return 1;
    }
  }
  return 0;
}
int hashMapInfos::trouve ( string key )
{
  long searchKey=hashValue ( key );
  long foundKey;;
//       vector<infosHasher>::const_iterator l_hasher=m_hasher.begin();
  for ( vector<infosHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ ) {
    foundKey= ( *l_hasher ).getHashKey();
    if ( searchKey == foundKey ) {
      return 1;
    }
  }
  return 0;
}

/**
 * long hashMapInfos::hashValue ( string key )
 * @param key
 * @return
 */
long hashMapInfos::hashValue ( string key )
{
  locale loc;                 // the "C" locale
  const collate<char>& coll = use_facet<collate<char> >(loc);
  return coll.hash(key.data(),key.data()+key.length());
// 	boost::hash<string> hasher;
//         return hasher ( key );
}
/**
 * void hashMapInfos::addHasher ( string key, string value )
 * @param key
 * @param value
 */
void hashMapInfos::addHasher ( string key, vector<int>  value )
{
  if ( trouve ( hashValue ( key ) ) ==0 ) {
//         cerr << "ICI1" <<endl;
    infosHasher H ( hashValue ( key ),key,value );
//         cerr <<" "<< hashValue ( key )<<" "<< key<<" "<<value <<endl;
//         cerr << "ICI2" <<endl;

    m_hasher.push_back ( H );
  }
}
void hashMapInfos::addValue ( string key, vector<int>  value )
{
  addHasher ( key, value );
}
infosHasher hashMapInfos::getHasher ( string key )
{
  long searchKey=hashValue ( key );
  long foundKey;
//       vector<infosHasher>::const_iterator l_hasher=m_hasher.begin();
  for ( vector<infosHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ ) {
    foundKey= ( *l_hasher ).getHashKey();
    if ( searchKey == foundKey ) {
      return ( *l_hasher );
    }
  }
  vector<int> temp;
  infosHasher defaut(0,"",temp);
  return defaut;
}
vector<int> hashMapInfos::getValue ( string key )
{
  long searchKey=hashValue ( key );
  long foundKey;
  vector<int> retour;
//       vector<infosHasher>::const_iterator l_hasher=m_hasher.begin();
  for ( vector<infosHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ ) {
    foundKey= ( *l_hasher ).getHashKey();
    if ( searchKey == foundKey ) {
//         cerr <<"value found : " << key<<"|"<< ( *l_hasher ).getValue()<<endl;
      return ( *l_hasher ).getValue();
    }
  }
  return retour;
}
//     string hashMapInfos::searchValue ( string value )
//     {
// //       long searchKey=hashValue ( key );
// //       long foundKey;
//       vector<int> foundValue;
//
// //       vector<infosHasher>::const_iterator l_hasher=m_hasher.begin();
//       for ( vector<infosHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ )
//       {
//         foundValue= ( *l_hasher ).getValue();
// /*        if ( foundValue.compare ( value ) == 0 )
//         {
//           return ( *l_hasher ).getKey();
//         }*/
//       }
//       return "";
//     }
//

void hashMapInfos::setValue ( string key , vector<int>  value )
{
  long searchKey=hashValue ( key );
  long foundKey;
//       vector<infosHasher>::const_iterator l_hasher=m_hasher.begin();
  for ( vector<infosHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ ) {
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
void hashMapInfos::printHash()
{
  for ( vector<infosHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ ) {
//         cout << ( *l_hasher ).getHashKey() <<" | "<< ( *l_hasher ).getKey() << " | " << ( *l_hasher ).getValue() << endl;
  }
}



//     long hashValue(string key){}

}

