//
//  Util.h
//  fuzzy-match
//
//  Created by Hieu Hoang on 25/07/2012.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#ifndef fuzzy_match_Util_h
#define fuzzy_match_Util_h

#include <vector>
#include <sstream>
#include "Vocabulary.h"

class SentenceAlignment;

void load_corpus( const char* fileName, std::vector< std::vector< WORD_ID > > &corpus );
void load_target( const char* fileName, std::vector< std::vector< SentenceAlignment > > &corpus);
void load_alignment( const char* fileName, std::vector< std::vector< SentenceAlignment > > &corpus );

/**
 * Convert vector of type T to string
 */
template <typename T>
std::string Join(const std::string& delimiter, const std::vector<T>& items)
{
  std::ostringstream outstr;
  if(items.size() == 0) return "";
  outstr << items[0];
  for(unsigned int i = 1; i < items.size(); i++)
    outstr << delimiter << items[i];
  return outstr.str();
}

//! convert string to variable of type T. Used to reading floats, int etc from files
template<typename T>
inline T Scan(const std::string &input)
{
  std::stringstream stream(input);
  T ret;
  stream >> ret;
  return ret;
}

//! convert vectors of string to vectors of type T variables
template<typename T>
inline std::vector<T> Scan(const std::vector< std::string > &input)
{
  std::vector<T> output(input.size());
  for (size_t i = 0 ; i < input.size() ; i++) {
    output[i] = Scan<T>( input[i] );
  }
  return output;
}

inline std::vector<std::string> Tokenize(const std::string& str,
                                         const std::string& delimiters = " \t")
{
  std::vector<std::string> tokens;
  // Skip delimiters at beginning.
  std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  std::string::size_type pos     = str.find_first_of(delimiters, lastPos);
  
  while (std::string::npos != pos || std::string::npos != lastPos) {
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
  }
  
  return tokens;
}

template<typename T>
inline std::vector<T> Tokenize( const std::string &input
                               , const std::string& delimiters = " \t")
{
  std::vector<std::string> stringVector = Tokenize(input, delimiters);
  return Scan<T>( stringVector );
}


#endif
