/*
 *  Util.h
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef UTIL_H
#define UTIL_H

using namespace std;

#include <limits>
#define US_NOSET (numeric_limits<unsigned short>::max())

#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <string>

#include "ScoreStats.h"
#include "FeatureStats.h"

class ScoreStats;
class FeatureStats;

#ifdef TRACE_ENABLE
#define TRACE_ERR(str) { std::cerr << str; }
#else
#define TRACE_ERR(str) { }
#endif

#define DELIMITER_SYMBOL " "

int getNextPound(std::string &theString, std::string &substring, const std::string delimiter=DELIMITER_SYMBOL);

template<typename T>
inline T Scan(const std::string &input)
{
	 std::stringstream stream(input);
	 T ret;
	 stream >> ret;
	 return ret;
};

template<typename T> 
int packVariable(char *buffer, size_t &bufferlen, T theVariable)
{
	size_t variable_size = sizeof(T);
	memcpy(buffer + bufferlen, (char*) &theVariable, variable_size);
	bufferlen += variable_size;
	return variable_size;
};

template<typename T> 
int unpackVariable(char *buffer, size_t &bufferlen, T &theVariable)
{
	size_t variable_size = sizeof(T);
	theVariable = *((T*)(buffer + bufferlen));
	bufferlen += variable_size;
	return variable_size;
};

template<typename T>
int packVector(char *buffer, size_t &bufferlen, vector<T> theVector)
{
	int vector_size = packVariable(buffer, bufferlen, theVector.size());
	
	for (int i = 0; i < theVector.size(); i++)
		vector_size += packVariable(buffer, bufferlen, theVector.at(i));		
	
	return vector_size;
};

template<typename T>
int unpackVector(char *buffer, size_t &bufferlen, vector<T> &theVector)
{
	int vector_size;
	int vector_memsize = unpackVariable(buffer, bufferlen, vector_size);
	
	theVector.clear();
	T theVariable;
	for (int i = 0; i < vector_size; i++)
	{
		vector_memsize += unpackVariable(buffer, bufferlen, theVariable);
		theVector.push_back(theVariable);
	}
	
	return vector_memsize;
};

#endif