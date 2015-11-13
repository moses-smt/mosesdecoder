/*********************************
tercpp: an open-source Translation Edit Rate (TER) scorer tool for Machine Translation.

Copyright 2010-2013, Christophe Servan, LIUM, University of Le Mans, France
Contact: christophe.servan@lium.univ-lemans.fr

The tercpp tool and library are free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the licence, or
(at your option) any later version.

This program and library are distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
**********************************/
#include "hashMapInfos.h"

// The following class defines a hash function for strings


using namespace std;

namespace TERCPPNS_HashMapSpace
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
string hashMapInfos::toString ()
{
  stringstream to_return;
  for ( vector<infosHasher>:: iterator l_hasher = m_hasher.begin() ; l_hasher != m_hasher.end() ; l_hasher++ ) {
    to_return << (*l_hasher).toString();
    //         cout << ( *l_hasher ).getHashKey() <<" | "<< ( *l_hasher ).getKey() << " | " << ( *l_hasher ).getValue() << endl;
  }
  return to_return.str();
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

