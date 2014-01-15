// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#ifndef moses_Util_h
#define moses_Util_h

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cassert>
#include <limits>
#include <map>
#include <cstdlib>
#include <cstring>
#include "util/exception.hh"
#include "TypeDef.h"

namespace Moses
{

/** Outputting debugging/verbose information to stderr.
 * Use TRACE_ENABLE flag to redirect tracing output into oblivion
 * so that you can output your own ad-hoc debugging info.
 * However, if you use stderr directly, please delete calls to it once
 * you finished debugging so that it won't clutter up.
 * Also use TRACE_ENABLE to turn off output of any debugging info
 * when compiling for a gui front-end so that running gui won't generate
 * output on command line
 * */
#ifdef TRACE_ENABLE
#define TRACE_ERR(str) do { std::cerr << str; } while (false)
#else
#define TRACE_ERR(str) do {} while (false)
#endif

/** verbose macros
 * */
#define VERBOSE(level,str) { if (StaticData::Instance().GetVerboseLevel() >= level) { TRACE_ERR(str); } }
#define IFVERBOSE(level) if (StaticData::Instance().GetVerboseLevel() >= level)

#if __GNUC__ == 4 && __GNUC_MINOR__ == 8 && (__GNUC_PATCHLEVEL__ == 1 || __GNUC_PATCHLEVEL__ == 2)
// gcc nth_element() bug
#define NTH_ELEMENT3(begin, middle, end) std::sort(begin, end)
#define NTH_ELEMENT4(begin, middle, end, orderer) std::sort(begin, end, orderer)
#else
#define NTH_ELEMENT3(begin, middle, end) std::nth_element(begin, middle, end)
#define NTH_ELEMENT4(begin, middle, end, orderer) std::nth_element(begin, middle, end, orderer)
#endif

//! delete white spaces at beginning and end of string
const std::string Trim(const std::string& str, const std::string dropChars = " \t\n\r");
const std::string ToLower(const std::string& str);

//! get string representation of any object/variable, as long as it can pipe to a stream
template<typename T>
inline std::string SPrint(const T &input)
{
  std::stringstream stream("");
  stream << input;
  return stream.str();
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

//! Specialisation to understand yes/no y/n true/false 0/1
template<>
bool Scan<bool>(const std::string &input);

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

//! speeded up version of above
template<typename T>
inline void Scan(std::vector<T> &output, const std::vector< std::string > &input)
{
  output.resize(input.size());
  for (size_t i = 0 ; i < input.size() ; i++) {
    output[i] = Scan<T>( input[i] );
  }
}

/** replace all occurrences of todelStr in str with the string toaddStr */
inline std::string Replace(const std::string& str,
                           const std::string& todelStr,
                           const std::string& toaddStr)
{
  size_t pos=0;
  std::string newStr=str;
  while ((pos=newStr.find(todelStr,pos))!=std::string::npos) {
    newStr.replace(pos++,todelStr.size(),toaddStr);
  }
  return newStr;
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

// speeded up version of above
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

//! tokenise input string to vector of type T
template<typename T>
inline std::vector<T> Tokenize( const std::string &input
                                , const std::string& delimiters = " \t")
{
  std::vector<std::string> stringVector = Tokenize(input, delimiters);
  return Scan<T>( stringVector );
}

// speeded up version of above
template<typename T>
inline void Tokenize( std::vector<T> &output
                      , const std::string &input
                      , const std::string& delimiters = " \t")
{
  std::vector<std::string> stringVector;
  Tokenize(stringVector, input, delimiters);
  return Scan<T>(output, stringVector );
}

inline std::vector<std::string> TokenizeMultiCharSeparator(
  const std::string& str,
  const std::string& separator)
{
  std::vector<std::string> tokens;

  size_t pos = 0;
  // Find first "non-delimiter".
  std::string::size_type nextPos     = str.find(separator, pos);

  while (nextPos != std::string::npos) {
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(pos, nextPos - pos));
    // Skip delimiters.  Note the "not_of"
    pos = nextPos + separator.size();
    // Find next "non-delimiter"
    nextPos	= str.find(separator, pos);
  }
  tokens.push_back(str.substr(pos, nextPos - pos));

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

//! transform prob to natural log score
inline float TransformScore(float prob)
{
  return log(prob);
}

//! transform natural log score to prob. Not currently used
inline float UntransformScore(float score)
{
  return exp(score);
}

//! irst number are in log 10, transform to natural log
inline float TransformLMScore(float irstScore)
{
  return irstScore * 2.30258509299405f;
}

inline float UntransformLMScore(float logNScore)
{
  // opposite of above
  return logNScore / 2.30258509299405f;
}

//! make sure score doesn't fall below LOWEST_SCORE
inline float FloorScore(float logScore)
{
  return (std::max)(logScore , LOWEST_SCORE);
}

/** convert prob vector to log prob and calc inner product with weight vector.
 * At least, that's what I think it does, fn is only 9 lines but can't figure out what it does.
 * Not sure whether give zens a medal for being a genius, or shoot him for writing unreadable code. Mabe both...
 */
inline float CalcTranslationScore(const std::vector<float> &probVector,
                                  const std::vector<float> &weightT)
{
  UTIL_THROW_IF2(weightT.size() != probVector.size(),
		  "Weight and score vector sizes not the same");
  float rv=0.0;
  for(float const *sb=&probVector[0],*se=sb+probVector.size(),*wb=&weightT[0];
      sb!=se; ++sb, ++wb)
    rv += TransformScore(*sb) * (*wb);
  return rv;
}

/** declaration of ToString() function to go in header for each class.
 *	This function, as well as the operator<< fn for each class, is
 *	for debugging purposes only. The output format is likely to change from
 *	time-to-time as classes are updated so shouldn't be relied upon
 *	for any decoding algorithm
*/
#define TO_STRING()	 std::string ToString() const;

//! definition of ToString() function to go in .cpp file. Can be used for any class that can be piped to a stream
#define TO_STRING_BODY(CLASS) 	\
	std::string CLASS::ToString() const	\
	{															\
		std::stringstream out;			\
		out << *this;								\
		return out.str();						\
	}															\
 
//! delete and remove every element of a collection object such as set, list etc
template<class COLL>
void RemoveAllInColl(COLL &coll)
{
  for (typename COLL::const_iterator iter = coll.begin() ; iter != coll.end() ; ++iter) {
    delete (*iter);
  }
  coll.clear();
}

//! delete and remove every element of map
template<class COLL>
void RemoveAllInMap(COLL &coll)
{
  for (typename COLL::const_iterator iter = coll.begin() ; iter != coll.end() ; ++iter) {
    delete (iter->second);
  }
  coll.clear();
}


//! x-platform reference to temp folder
std::string GetTempFolder();
//! MD5 hash of a file
std::string GetMD5Hash(const std::string &filePath);

//! save memory by getting rid of spare, unused elements in a collection
template<typename T>
inline void ShrinkToFit(T& v)
{
  if(v.capacity()>v.size())
    T(v).swap(v);
  assert(v.capacity()==v.size());
}

bool FileExists(const std::string& filePath);

// A couple of utilities to measure decoding time
void ResetUserTime();
void PrintUserTime(const std::string &message);
double GetUserTime();

// dump SGML parser for <seg> tags
std::map<std::string, std::string> ProcessAndStripSGML(std::string &line);

std::string PassthroughSGML(std::string &line, const std::string tagName,const std::string& lbrackStr="<", const std::string& rbrackStr=">");

/**
 * Returns the first string bounded by the delimiters (default delimiters are " " and "\t")i starting from position first_pos
 * and and stores the starting position of the next string (in first_str)
 */
inline std::string GetFirstString(const std::string& str, int& first_pos,  const std::string& delimiters = " \t")
{

  std::string first_str;
  // Skip delimiters at beginning.
  std::string::size_type lastPos = str.find_first_not_of(delimiters, first_pos);

  // Find first "non-delimiter".
  std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

  if (std::string::npos != pos || std::string::npos != lastPos) {

    first_str = str.substr(lastPos, pos - lastPos);

    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);

  }

  first_pos = lastPos;
  return first_str;
}

template<class T>
T log_sum (T log_a, T log_b)
{
  T v;
  if (log_a < log_b) {
    v = log_b+log ( 1 + exp ( log_a-log_b ));
  } else {
    v = log_a+log ( 1 + exp ( log_b-log_a ));
  }
  return ( v );
}


}

#endif
