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
#include "hashMapStringInfos.h"

// The following class defines a hash function for strings


using namespace std;

namespace TERCPPNS_HashMapSpace
{
//     hashMapStringInfos::hashMap();
/*    hashMapStringInfos::~hashMap()
{
//       vector<stringInfosHasher>::const_iterator del = m_hasher.begin();
  for ( vector<stringInfosHasher>::const_iterator del=m_hasher.begin(); del != m_hasher.end(); del++ )
  {
    delete(*del);
  }
}*/
/**
* int hashMapStringInfos::trouve ( long searchKey )
* @param searchKey
* @return
*/
int hashMapStringInfos::trouve ( long searchKey )
{
  long foundKey;
  //       vector<stringInfosHasher>::const_iterator l_hasher=m_hasher.begin();
  for ( vector<stringInfosHasher>:: iterator l_hasher = m_hasher.begin() ; l_hasher != m_hasher.end() ; l_hasher++ ) {
    foundKey = ( *l_hasher ).getHashKey();
    if ( searchKey == foundKey ) {
      return 1;
    }
  }
  return 0;
}

int hashMapStringInfos::trouve ( string key )
{
  long searchKey = hashValue ( key );
  long foundKey;;
  //       vector<stringInfosHasher>::const_iterator l_hasher=m_hasher.begin();
  for ( vector<stringInfosHasher>:: iterator l_hasher = m_hasher.begin() ; l_hasher != m_hasher.end() ; l_hasher++ ) {
    foundKey = ( *l_hasher ).getHashKey();
    if ( searchKey == foundKey ) {
      return 1;
    }
  }
  return 0;
}

/**
* long hashMapStringInfos::hashValue ( string key )
* @param key
* @return
*/
long hashMapStringInfos::hashValue ( string key )
{
  locale loc;                 // the "C" locale
  const collate<char>& coll = use_facet<collate<char> > ( loc );
  return coll.hash ( key.data(), key.data() + key.length() );
// 	boost::hash<string> hasher;
// 	return hasher ( key );
}
/**
* void hashMapStringInfos::addHasher ( string key, string value )
* @param key
* @param value
*/
void hashMapStringInfos::addHasher ( string key, vector<string>  value )
{
  if ( trouve ( hashValue ( key ) ) == 0 ) {
    //         cerr << "ICI1" <<endl;
    stringInfosHasher H ( hashValue ( key ), key, value );
    //         cerr <<" "<< hashValue ( key )<<" "<< key<<" "<<value <<endl;
    //         cerr << "ICI2" <<endl;

    m_hasher.push_back ( H );
  }
}
void hashMapStringInfos::addValue ( string key, vector<string>  value )
{
  addHasher ( key, value );
}
stringInfosHasher hashMapStringInfos::getHasher ( string key )
{
  long searchKey = hashValue ( key );
  long foundKey;
  //       vector<stringInfosHasher>::const_iterator l_hasher=m_hasher.begin();
  for ( vector<stringInfosHasher>:: iterator l_hasher = m_hasher.begin() ; l_hasher != m_hasher.end() ; l_hasher++ ) {
    foundKey = ( *l_hasher ).getHashKey();
    if ( searchKey == foundKey ) {
      return ( *l_hasher );
    }
  }
  vector<string> tmp;
  stringInfosHasher defaut ( 0, "", tmp );
  return defaut;
}
vector<string> hashMapStringInfos::getValue ( string key )
{
  long searchKey = hashValue ( key );
  long foundKey;
  vector<string> retour;
  //       vector<stringInfosHasher>::const_iterator l_hasher=m_hasher.begin();
  for ( vector<stringInfosHasher>:: iterator l_hasher = m_hasher.begin() ; l_hasher != m_hasher.end() ; l_hasher++ ) {
    foundKey = ( *l_hasher ).getHashKey();
    if ( searchKey == foundKey ) {
      //         cerr <<"value found : " << key<<"|"<< ( *l_hasher ).getValue()<<endl;
      return ( *l_hasher ).getValue();
    }
  }
  return retour;
}
//     string hashMapStringInfos::searchValue ( string value )
//     {
// //       long searchKey=hashValue ( key );
// //       long foundKey;
//       vector<int> foundValue;
//
// //       vector<stringInfosHasher>::const_iterator l_hasher=m_hasher.begin();
//       for ( vector<stringInfosHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ )
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

void hashMapStringInfos::setValue ( string key , vector<string>  value )
{
  long searchKey = hashValue ( key );
  long foundKey;
  //       vector<stringInfosHasher>::const_iterator l_hasher=m_hasher.begin();
  for ( vector<stringInfosHasher>:: iterator l_hasher = m_hasher.begin() ; l_hasher != m_hasher.end() ; l_hasher++ ) {
    foundKey = ( *l_hasher ).getHashKey();
    if ( searchKey == foundKey ) {
      ( *l_hasher ).setValue ( value );
      //           return ( *l_hasher ).getValue();
    }
  }
}

string hashMapStringInfos::toString ()
{
  stringstream to_return;
  for ( vector<stringInfosHasher>:: iterator l_hasher = m_hasher.begin() ; l_hasher != m_hasher.end() ; l_hasher++ ) {
    to_return << (*l_hasher).toString();
    //         cout << ( *l_hasher ).getHashKey() <<" | "<< ( *l_hasher ).getKey() << " | " << ( *l_hasher ).getValue() << endl;
  }
  return to_return.str();
}

/**
*
*/
void hashMapStringInfos::printHash()
{
  for ( vector<stringInfosHasher>:: iterator l_hasher = m_hasher.begin() ; l_hasher != m_hasher.end() ; l_hasher++ ) {
    //         cout << ( *l_hasher ).getHashKey() <<" | "<< ( *l_hasher ).getKey() << " | " << ( *l_hasher ).getValue() << endl;
  }
}
vector< stringInfosHasher > hashMapStringInfos::getHashMap()
{
  return m_hasher;
}



//     long hashValue(string key){}

}

