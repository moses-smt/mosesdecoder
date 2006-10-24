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

#pragma once

#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <limits>
#include "TypeDef.h"

/** Outputting debugging information. define TRACE_ENABLE as compiler flag (-D for gcc) to enable output
 *  change to send to a log file if required
 * */
#ifdef TRACE_ENABLE
#define TRACE_ERR(str) { std::cerr << str; }
#else
#define TRACE_ERR(str) { }
#endif

/** verbose macros
 * */
#define VERBOSE(level,str) { if (StaticData::Instance()->GetVerboseLevel() >= level) { std::cerr << str; } }
#define IFVERBOSE(level) if (StaticData::Instance()->GetVerboseLevel() >= level)

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

//! Specialisation for performance
template<>
inline int Scan<int>(const std::string &input)
{
	return atoi(input.c_str());
}

//! Specialisation for performance
template<>
inline float Scan<float>(const std::string &input)
{
	return (float) atof(input.c_str());
}

//! Specialisation to understand yes/no y/n true/false 0/1
template<>
bool Scan<bool>(const std::string &input);

//! convert vectors of string to vectors of type T variables
template<typename T>
inline std::vector<T> Scan(const std::vector< std::string > &input)
{
	std::vector<T> output(input.size());
	for (size_t i = 0 ; i < input.size() ; i++)
	{
		output[i] = Scan<T>( input[i] );
	}
	return output;
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

	while (std::string::npos != pos || std::string::npos != lastPos)
	{
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
inline std::vector<T> Tokenize( const std::string &input
															 , const std::string& delimiters = " \t") 
{
	std::vector<std::string> stringVector = Tokenize(input, delimiters);
	return Scan<T>( stringVector );
}

inline std::vector<std::string> TokenizeMultiCharSeparator(
																				const std::string& str,
																				const std::string& separator)
{
	std::vector<std::string> tokens;

	size_t pos = 0;
	// Find first "non-delimiter".
	std::string::size_type nextPos     = str.find(separator, pos);

	while (nextPos != std::string::npos)
	{
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
inline float TransformIRSTScore(float irstScore)
{ 
	return irstScore * 2.30258509299405f;
}

inline float UntransformIRSTScore(float logNScore)
{ // opposite of above
	return logNScore / 2.30258509299405f;
}

//! make sure LM score doesn't fall below LOWEST_SCORE
inline float FloorIRSTScore(float irstScore)
{
	return (std::max)(irstScore , LOWEST_SCORE);
}

//! Should SRI & IRST transform functions be merged ???
inline float TransformSRIScore(float sriScore)
{
	return sriScore * 2.30258509299405f;
}

inline float UntransformSRIScore(float logNScore)
{ // opposite of above
	return logNScore / 2.30258509299405f;
}

inline float FloorSRIScore(float sriScore)
{
	return (std::max)(sriScore, LOWEST_SCORE);
}

/** convert prob vector to log prob and calc inner product with weight vector.
 * At least, that's what I think it does, fn is only 9 lines but can't figure out what it does.
 * Not sure whether give zens a medal for being a genius, or shoot him for writing unreadable code. Mabe both...
 */
inline float CalcTranslationScore(const std::vector<float> &probVector, 
																	const std::vector<float> &weightT) 
{
	assert(weightT.size()==probVector.size());
	float rv=0.0;
	for(float const *sb=&probVector[0],*se=sb+probVector.size(),*wb=&weightT[0];
			sb!=se; ++sb, ++wb)
		rv += TransformScore(*sb) * (*wb);
	return rv;
}

//! declaration of ToString() function to go in header for each class. 
#define TO_STRING()	 const char* ToString() const;

//! definition of ToString() function to go in .cpp file. Can be used for any class that can be piped to a stream
#define TO_STRING_BODY(CLASS) 	\
	const char* CLASS::ToString() const	\
	{															\
		std::stringstream out;			\
		out << *this;								\
		return out.str().c_str();						\
	}															\

//! delete and remove every element of a collection object such as map, set, list etc
template<class COLL>
void RemoveAllInColl(COLL &coll)
{
	for (typename COLL::const_iterator iter = coll.begin() ; iter != coll.end() ; ++iter)
	{
		delete (*iter);
	}
	coll.clear();

}

//! x-platform reference to temp folder
std::string GetTempFolder();
//! Create temp file and return output stream and full file path as arguments
void CreateTempFile(std::ofstream  &fileStream, std::string &filePath);
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
//! delete white spaces at beginning and end of string
const std::string Trim(const std::string& str, const std::string dropChars = " \t\n\r");
const std::string ToLower(const std::string& str);

// A couple of utilities to measure decoding time
#ifdef WIN32
inline void ResetUserTime() {}
inline void PrintUserTime(std::ostream &out, const std::string &message="") {}
#else
void ResetUserTime();
void PrintUserTime(std::ostream &out, const std::string &message="");
#endif
