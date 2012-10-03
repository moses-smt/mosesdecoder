#ifndef moses_DynSAInclude_utils_h
#define moses_DynSAInclude_utils_h

#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>
#include <cctype>
#include <cmath>
#include <cstring>

//! @todo ask abby2
class Utils
{
public:
  static void trim(std::string& str, const std::string dropChars = " \t\n\r") {
    str.erase(str.find_last_not_of(dropChars)+1);
    str.erase(0, str.find_first_not_of(dropChars));
  }
  static void rtrim(std::string& str, const std::string dropChars = " \t\n\r") {
    str.erase(str.find_last_not_of(dropChars)+1);
  }
  static void ltrim(std::string& str, const std::string dropChars = " \t\n\r") {
    str.erase(0, str.find_first_not_of(dropChars));
  }
  static std::string IntToStr(int integer) {
    std::ostringstream stream;
    stream << integer;
    return stream.str();
  }
  static int splitToStr(const char * str,
                        std::vector<std::string> & items,
                        const char * delm = "\t") {
    char * buff = const_cast<char *>(str);
    items.clear();
    char * pch = strtok(buff, delm);
    while( pch != NULL ) {
      items.push_back(pch);
      pch = strtok(NULL, delm);
    }
    return items.size();
  }
  static int splitToStr(std::string buff,
                        std::vector<std::string> & items,
                        std::string delm = "\t") {
    std::string cp = buff.substr();
    return splitToStr(cp.c_str(), items, delm.c_str());
  }
  static int splitToInt(std::string buff, std::vector<int>& items,
                        std::string delm = ",") {
    items.clear();
    std::vector<std::string> tmpVector(0);
    int i = 0;
    i = splitToStr(buff.c_str(), tmpVector, delm.c_str());
    if( i > 0 )
      for( int j = 0; j < i; j++ )
        items.push_back(atoi(tmpVector[j].c_str()));
    return i;
  }
  static void strToLowercase(std::string& str) {
    for(unsigned i=0; i < str.length(); i++) {
      str[i] = tolower(str[i]);
    }
  }
  // TODO: interface with decent PRG
  template<typename T>
  static T rand(T mod_bnd = 0) {
    T random = 0;
    if(sizeof(T) <= 4) {
      random = static_cast<T>(std::rand());
    } else if(sizeof(T) == 8) {
      random = static_cast<T>(std::rand());
      random <<= 31;
      random <<= 1;
      random |= static_cast<T>(std::rand());
    }
    if(mod_bnd != 0)
      return random % mod_bnd;
    else return random;
  }
};

#endif
