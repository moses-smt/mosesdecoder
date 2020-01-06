#pragma once

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <limits>
#include <sstream>
#include <vector>
#include <queue>
#include <cmath>
#include <stdlib.h>
#include "../TypeDef.h"
#include "util/exception.hh"

namespace Moses2
{

#ifdef TRACE_ERR
#undef TRACE_ERR
#endif
#ifdef TRACE_ENABLE
#define TRACE_ERR(str) do { std::cerr << str; } while (false)
#else
#define TRACE_ERR(str) do {} while (false)
#endif

////////////////////////////////////////////////////

template<typename T>
class UnorderedComparer
{
public:
  size_t operator()(const T* obj) const {
    return obj->hash();
  }

  bool operator()(const T* a, const T* b) const {
    return a->hash() == b->hash();
  }

};

////////////////////////////////////////////////////


template<typename T>
void Init(T arr[], size_t size, const T &val)
{
  for (size_t i = 0; i < size; ++i) {
    arr[i] = val;
  }
}

//! delete white spaces at beginning and end of string
inline std::string Trim(const std::string& str, const std::string dropChars =
                          " \t\n\r")
{
  std::string res = str;
  res.erase(str.find_last_not_of(dropChars) + 1);
  return res.erase(0, res.find_first_not_of(dropChars));
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

//! just return input
template<>
inline std::string Scan<std::string>(const std::string &input)
{
  return input;
}

template<>
inline SCORE Scan<SCORE>(const std::string &input)
{
  SCORE ret = atof(input.c_str());
  return ret;
}

//! Specialisation to understand yes/no y/n true/false 0/1
template<>
bool Scan<bool>(const std::string &input);

template<>
inline S2TParsingAlgorithm Scan<S2TParsingAlgorithm>(const std::string &input)
{
  return (S2TParsingAlgorithm) Scan<size_t>(input);
}

template<>
inline SourceLabelOverlap Scan<SourceLabelOverlap>(const std::string &input)
{
  return (SourceLabelOverlap) Scan<size_t>(input);
}

template<>
inline SearchAlgorithm Scan<SearchAlgorithm>(const std::string &input)
{
  return (SearchAlgorithm) Scan<size_t>(input);
}

template<>
inline XmlInputType Scan<XmlInputType>(const std::string &input)
{
  XmlInputType ret;
  if (input=="exclusive") ret = XmlExclusive;
  else if (input=="inclusive") ret = XmlInclusive;
  else if (input=="constraint") ret = XmlConstraint;
  else if (input=="ignore") ret = XmlIgnore;
  else if (input=="pass-through") ret = XmlPassThrough;
  else {
    UTIL_THROW2("Unknown XML input type");
  }

  return ret;
}

template<>
inline InputTypeEnum Scan<InputTypeEnum>(const std::string &input)
{
  return (InputTypeEnum) Scan<size_t>(input);
}

template<>
inline WordAlignmentSort Scan<WordAlignmentSort>(const std::string &input)
{
  return (WordAlignmentSort) Scan<size_t>(input);
}

//! convert vectors of string to vectors of type T variables
template<typename T>
inline std::vector<T> Scan(const std::vector<std::string> &input)
{
  std::vector<T> output(input.size());
  for (size_t i = 0; i < input.size(); i++) {
    output[i] = Scan<T>(input[i]);
  }
  return output;
}

//! speeded up version of above
template<typename T>
inline void Scan(std::vector<T> &output, const std::vector<std::string> &input)
{
  output.resize(input.size());
  for (size_t i = 0; i < input.size(); i++) {
    output[i] = Scan<T>(input[i]);
  }
}

/** tokenise input string to vector of string. each element has been separated by a character in the delimiters argument.
 The separator can only be 1 character long. The default delimiters are space or tab
 */
inline std::vector<std::string> Tokenize(const std::string& str,
    const std::string& delimiters = " \t")
{
  std::vector<std::string> tokens;
  // Skip delimiters at beginning.
  std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  std::string::size_type pos = str.find_first_of(delimiters, lastPos);

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

//! tokenise input string to vector of type T
template<typename T>
inline std::vector<T> Tokenize(const std::string &input,
                               const std::string& delimiters = " \t")
{
  std::vector<std::string> stringVector = Tokenize(input, delimiters);
  return Scan<T>(stringVector);
}

/** only split of the first delimiter. Used by class FeatureFunction for parse key=value pair.
 * Value may have = character
 */
inline std::vector<std::string> TokenizeFirstOnly(const std::string& str,
    const std::string& delimiters = " \t")
{
  std::vector<std::string> tokens;
  std::string::size_type pos = str.find_first_of(delimiters);

  if (std::string::npos != pos) {
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(0, pos));
    tokens.push_back(str.substr(pos + 1, str.size() - pos - 1));
  } else {
    tokens.push_back(str);
  }

  return tokens;
}

inline std::vector<std::string> TokenizeMultiCharSeparator(
  const std::string& str, const std::string& separator)
{
  std::vector<std::string> tokens;

  size_t pos = 0;
  // Find first "non-delimiter".
  std::string::size_type nextPos = str.find(separator, pos);

  while (nextPos != std::string::npos) {
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(pos, nextPos - pos));
    // Skip delimiters.  Note the "not_of"
    pos = nextPos + separator.size();
    // Find next "non-delimiter"
    nextPos = str.find(separator, pos);
  }
  tokens.push_back(str.substr(pos, nextPos - pos));

  return tokens;
}

// speeded up version of above
inline void TokenizeMultiCharSeparator(std::vector<std::string> &output,
                                       const std::string& str, const std::string& separator)
{
  size_t pos = 0;
  // Find first "non-delimiter".
  std::string::size_type nextPos = str.find(separator, pos);

  while (nextPos != std::string::npos) {
    // Found a token, add it to the vector.
    output.push_back(Trim(str.substr(pos, nextPos - pos)));
    // Skip delimiters.  Note the "not_of"
    pos = nextPos + separator.size();
    // Find next "non-delimiter"
    nextPos = str.find(separator, pos);
  }
  output.push_back(Trim(str.substr(pos, nextPos - pos)));
}

//! get string representation of any object/variable, as long as it can pipe to a stream
template<typename T>
inline std::string SPrint(const T &input)
{
  std::stringstream stream("");
  stream << input;
  return stream.str();
}

//! irst number are in log 10, transform to natural log
inline float TransformLMScore(float irstScore)
{
  return irstScore * 2.30258509299405f;
}

//! transform prob to natural log score
inline float TransformScore(float prob)
{
  return log(prob);
}

//! make sure score doesn't fall below LOWEST_SCORE
inline float FloorScore(float logScore)
{
  return (std::max)(logScore, LOWEST_SCORE);
}

inline float UntransformLMScore(float logNScore)
{
  // opposite of above
  return logNScore / 2.30258509299405f;
}

inline bool FileExists(const std::string& filePath)
{
  std::ifstream ifs(filePath.c_str());
  return !ifs.fail();
}

const std::string ToLower(const std::string& str);

//! delete and remove every element of a collection object such as set, list etc
template<class COLL>
void RemoveAllInColl(COLL &coll)
{
  for (typename COLL::const_iterator iter = coll.begin(); iter != coll.end();
       ++iter) {
    delete (*iter);
  }
  coll.clear();
}

template<typename T>
void Swap(T &a, T &b)
{
  T &c = a;
  a = b;
  b = c;
}

// grab the underlying contain of priority queue
template<class T, class S, class C>
S& Container(std::priority_queue<T, S, C>& q)
{
  struct HackedQueue: private std::priority_queue<T, S, C> {
    static S& Container(std::priority_queue<T, S, C>& q) {
      return q.*&HackedQueue::c;
    }
  };
  return HackedQueue::Container(q);
}

#define HERE __FILE__ << ":" << __LINE__

/** Enforce rounding */
inline void FixPrecision(std::ostream& stream, size_t size = 3)
{
  stream.setf(std::ios::fixed);
  stream.precision(size);
}

}

