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
#include <assert.h>

#ifdef TRACE_ENABLE
#define TRACE_ERR(str) { std::cerr << str; }
#else
#define TRACE_ERR(str) { }
#endif

template<typename T>
inline std::string SPrint(const T &input)
{
	std::stringstream stream("");
	stream << input;
	return stream.str();
}

template<typename T>
inline T Scan(const std::string &input)
{
	std::stringstream stream(input);
	T ret;
	stream >> ret;
	return ret;
}

template<>
inline int Scan<int>(const std::string &input)
{
	return atoi(input.c_str());
}

template<>
bool Scan<bool>(const std::string &input);

template<>
inline float Scan<float>(const std::string &input)
{
	return (float) atof(input.c_str());
}

// convert vectors
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

// tokenize then convert each element into another type
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

// transform prob to natural log score
inline float TransformScore(float prob)
{
	return log(prob);
}

// transform natural log score to prob. 
// not sure if needed
inline float UntransformScore(float score)
{
	return exp(score);
}

inline float TransformIRSTScore(float irstScore)
{ // irst number are in log 10
	// transform to natural log
	return irstScore * 2.30258509299405f;
}

inline float UntransformIRSTScore(float logNScore)
{ // opposite of above
	return logNScore / 2.30258509299405f;
}

inline float FloorIRSTScore(float irstScore)
{
	return (std::max)(irstScore , LOWEST_SCORE);
}


inline float TransformSRIScore(float sriScore)
{ // sri number are in log 10
	// transform to natural log
	return sriScore * 2.30258509299405f;
}

inline float UntransformSRIScore(float logNScore)
{ // opposite of above
	return logNScore / 2.30258509299405f;
}

inline float FloorSRIScore(float sriScore)
{
	return (std::max)(sriScore , LOWEST_SCORE);
}

inline float CalcTranslationScore(const std::vector<float> &scoreVector, 
																	const std::vector<float> &weightT) 
{
	assert(weightT.size()==scoreVector.size());
	float rv=0.0;
	for(float const *sb=&scoreVector[0],*se=sb+scoreVector.size(),*wb=&weightT[0];
			sb!=se;++sb,++wb)
		rv += TransformScore(*sb) * (*wb);
	return rv;
}

#define TO_STRING	 std::string ToString() const;

#define TO_STRING_BODY(CLASS) 	\
	std::string CLASS::ToString() const	\
	{															\
		std::stringstream out;			\
		out << *this;								\
		return out.str();						\
	}															\

template<class ITER, class COLL>
void RemoveAllInColl(COLL &coll)
{
	ITER iter;
	for (iter = coll.begin() ; iter != coll.end() ; ++iter)
	{
		delete (*iter);
	}
	coll.clear();

}

std::string GetTempFolder();
void CreateTempFile(std::ofstream  &fileStream, std::string &filePath);
std::string GetMD5Hash(const std::string &filePath);

template<typename T> inline void ShrinkToFit(T& v) {
  if(v.capacity()>v.size()) T(v).swap(v);assert(v.capacity()==v.size());}



