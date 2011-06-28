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

#include <stdexcept>
#include <limits>

#define US_NOSET (numeric_limits<unsigned short>::max())

#define MAX_LINE  1024

#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>

#include <fstream>
#include "gzfilebuf.h"

#include "Types.h"
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

int verboselevel();
int setverboselevel(int v);

int getNextPound(std::string &theString, std::string &substring, const std::string delimiter=DELIMITER_SYMBOL);

template<typename T>
inline T Scan(const std::string &input)
{
	 std::stringstream stream(input);
	 T ret;
	 stream >> ret;
	 return ret;
};

class inputfilestream : public std::istream
{
protected:
        std::streambuf *m_streambuf;
	bool _good;
public:
  
        inputfilestream(const std::string &filePath);
        ~inputfilestream();
	bool good(){return _good;}
        void close();
};

class outputfilestream : public std::ostream
{
protected:
        std::streambuf *m_streambuf;
	bool _good;
public:
  
        outputfilestream(const std::string &filePath);
        ~outputfilestream();
	bool good(){return _good;}
        void close();
};

template<typename T>
inline std::string stringify(T x)
{
	std::ostringstream o;
	if (!(o << x))
		throw std::runtime_error("stringify(template<typename T>)");
	return o.str();
}

// Utilities to measure decoding time
void ResetUserTime();
void PrintUserTime(const std::string &message);
double GetUserTime();

#endif

