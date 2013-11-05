
#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <ostream>
#include "TypeDef.h"

//! get string representation of any object/variable, as long as it can pipe to a stream
template<typename T>
inline std::string SPrint(const T &input)
{
  std::stringstream stream("");
  stream << input;
  return stream.str();
}

//! delete white spaces at beginning and end of string
const std::string Trim(const std::string& str, const std::string dropChars = " \t\n\r");

//! convert string to variable of type T. Used to reading floats, int etc from files
template<typename T>
inline T Scan(const std::string &input)
{
  std::stringstream stream(input);
  T ret;
  stream >> ret;
  return ret;
}

template<typename T>
inline void Scan(std::vector<T> &output, const std::vector< std::string > &input)
{
  output.resize(input.size());
  for (size_t i = 0 ; i < input.size() ; i++) {
    output[i] = Scan<T>( input[i] );
  }
}

inline void Tokenize(std::vector<std::string> &output
                     , const std::string& str
                     , const std::string& delimiters = " \t")
{
  // Skip delimiters at beginning.
  std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

  while (std::string::npos != pos || std::string::npos != lastPos) {
    // Found a token, add it to the vector.
    output.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
  }
}

template<typename T>
inline void Tokenize( std::vector<T> &output
                      , const std::string &input
                      , const std::string& delimiters = " \t")
{
  std::vector<std::string> stringVector;
  Tokenize(stringVector, input, delimiters);
  return Scan<T>(output, stringVector );
}


/** only split of the first delimiter. Used by class FeatureFunction for parse key=value pair.
 * Value may have = character
*/
inline std::vector<std::string> TokenizeFirstOnly(const std::string& str,
    const std::string& delimiters = " \t")
{
  std::vector<std::string> tokens;
  std::string::size_type pos     = str.find_first_of(delimiters);

  if (std::string::npos != pos) {
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(0, pos));
    tokens.push_back(str.substr(pos + 1, str.size() - pos  - 1));
  } else {
    tokens.push_back(str);
  }

  return tokens;
}

// speeded up version of above
inline void TokenizeMultiCharSeparator(std::vector<std::string> &output
                                       ,const std::string& str
                                       ,const std::string& separator)
{
  size_t pos = 0;
  // Find first "non-delimiter".
  std::string::size_type nextPos     = str.find(separator, pos);

  while (nextPos != std::string::npos) {
    // Found a token, add it to the vector.
    output.push_back(Trim(str.substr(pos, nextPos - pos)));
    // Skip delimiters.  Note the "not_of"
    pos = nextPos + separator.size();
    // Find next "non-delimiter"
    nextPos	= str.find(separator, pos);
  }
  output.push_back(Trim(str.substr(pos, nextPos - pos)));
}

inline SCORE FloorScore(SCORE logScore)
{
  return (std::max)(logScore , LOWEST_SCORE);
}

inline SCORE TransformScore(SCORE prob)
{
  return FloorScore(log(prob));
}

inline float TransformSRIScore(float sriScore)
{
  return sriScore * 2.30258509299405f;
}

/** Enforce rounding */
inline void Fix(std::ostream& stream, size_t size)
{
  stream.setf(std::ios::fixed);
  stream.precision(size);
}
