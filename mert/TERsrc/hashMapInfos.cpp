#include "hashMapInfos.h"

// The following class defines a hash function for strings


using namespace std;

namespace HashMapSpace
{
//     hashMapInfos::hashMap();
    /*    hashMapInfos::~hashMap()
        {
    //       std::vector<infosHasher>::const_iterator del = m_hasher.begin();
          for ( std::vector<infosHasher>::const_iterator del=m_hasher.begin(); del != m_hasher.end(); del++ )
          {
            delete(*del);
          }
        }*/
    /**
     * int hashMapInfos::trouve ( size_t searchKey )
     * @param searchKey
     * @return
     */
    int hashMapInfos::trouve ( size_t searchKey )
    {
        std::size_t foundKey;
//       std::vector<infosHasher>::const_iterator l_hasher=m_hasher.begin();
        for ( vector<infosHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ )
        {
            foundKey= ( *l_hasher ).getHashKey();
            if ( searchKey == foundKey )
            {
                return 1;
            }
        }
        return 0;
    }
    int hashMapInfos::trouve ( string key )
    {
        std::size_t searchKey=hashValue ( key );
        std::size_t foundKey;;
//       std::vector<infosHasher>::const_iterator l_hasher=m_hasher.begin();
        for ( vector<infosHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ )
        {
            foundKey= ( *l_hasher ).getHashKey();
            if ( searchKey == foundKey )
            {
                return 1;
            }
        }
        return 0;
    }

    /**
     * std::size_t hashMapInfos::hashValue ( std::string key )
     * @param key
     * @return
     */
    std::size_t hashMapInfos::hashValue ( std::string key )
    {
        boost::hash<std::string> hasher;
        return hasher ( key );
    }
    /**
     * void hashMapInfos::addHasher ( std::string key, std::string value )
     * @param key
     * @param value
     */
    void hashMapInfos::addHasher ( std::string key, vector<int>  value )
    {
        if ( trouve ( hashValue ( key ) ) ==0 )
        {
//         cerr << "ICI1" <<endl;
            infosHasher H ( hashValue ( key ),key,value );
//         cerr <<" "<< hashValue ( key )<<" "<< key<<" "<<value <<endl;
//         cerr << "ICI2" <<endl;

            m_hasher.push_back ( H );
        }
    }
    void hashMapInfos::addValue ( std::string key, vector<int>  value )
    {
        addHasher ( key, value );
    }
    infosHasher hashMapInfos::getHasher ( std::string key )
    {
        std::size_t searchKey=hashValue ( key );
        std::size_t foundKey;
//       std::vector<infosHasher>::const_iterator l_hasher=m_hasher.begin();
        for ( vector<infosHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ )
        {
            foundKey= ( *l_hasher ).getHashKey();
            if ( searchKey == foundKey )
            {
                return ( *l_hasher );
            }
        }
        vector<int> temp;
	infosHasher defaut(0,"",temp);
	return defaut;
    }
    vector<int> hashMapInfos::getValue ( std::string key )
    {
        std::size_t searchKey=hashValue ( key );
        std::size_t foundKey;
        vector<int> retour;
//       std::vector<infosHasher>::const_iterator l_hasher=m_hasher.begin();
        for ( vector<infosHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ )
        {
            foundKey= ( *l_hasher ).getHashKey();
            if ( searchKey == foundKey )
            {
//         cerr <<"value found : " << key<<"|"<< ( *l_hasher ).getValue()<<endl;
                return ( *l_hasher ).getValue();
            }
        }
        return retour;
    }
//     std::string hashMapInfos::searchValue ( std::string value )
//     {
// //       std::size_t searchKey=hashValue ( key );
// //       std::size_t foundKey;
//       vector<int> foundValue;
//
// //       std::vector<infosHasher>::const_iterator l_hasher=m_hasher.begin();
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

    void hashMapInfos::setValue ( std::string key , vector<int>  value )
    {
        std::size_t searchKey=hashValue ( key );
        std::size_t foundKey;
//       std::vector<infosHasher>::const_iterator l_hasher=m_hasher.begin();
        for ( vector<infosHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ )
        {
            foundKey= ( *l_hasher ).getHashKey();
            if ( searchKey == foundKey )
            {
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
        for ( vector<infosHasher>:: iterator l_hasher=m_hasher.begin() ; l_hasher!=m_hasher.end() ; l_hasher++ )
        {
//         cout << ( *l_hasher ).getHashKey() <<" | "<< ( *l_hasher ).getKey() << " | " << ( *l_hasher ).getValue() << endl;
        }
    }



//     std::size_t hashValue(std::string key){}

}

