/*
 *  Util.h
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef UTIL_H
#define UTIL_H

#include <cstdlib>
#include <stdexcept>
#include <limits>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>

#include "Types.h"

using namespace std;

#define US_NOSET (numeric_limits<unsigned short>::max())
#define MAX_LINE  1024

#ifdef TRACE_ENABLE
#define TRACE_ERR(str) { std::cerr << str; }
#else
#define TRACE_ERR(str) { }
#endif

const char kDefaultDelimiterSymbol[] = " ";

int verboselevel();
int setverboselevel(int v);

/**
 * Find the specified delimiter for the string 'str', and 'str' is assigned
 * to a substring object that starts at the position of first occurrence of
 * the delimiter in 'str'. 'substr' is copied from 'str' ranging from
 * the start position of 'str' to the position of first occurrence of
 * the delimiter.
 *
 * It returns the position of first occurrence in the queried string.
 * If the content is not found, std::string::npos is returned.
 */
size_t getNextPound(std::string &str, std::string &substr,
                    const std::string &delimiter = kDefaultDelimiterSymbol);

void split(const std::string &s, char delim, std::vector<std::string> &elems);

void Tokenize(const char *str, const char delim, std::vector<std::string> *res);

template<typename T>
inline T Scan(const std::string &input)
{
  std::stringstream stream(input);
  T ret;
  stream >> ret;
  return ret;
}

template<typename T>
inline std::string stringify(T x)
{
  std::ostringstream o;
  if (!(o << x))
    throw std::runtime_error("stringify(template<typename T>)");
  return o.str();
}

inline ScoreStatsType ConvertCharToScoreStatsType(const char *str)
{
  return std::atoi(str);
}

inline ScoreStatsType ConvertStringToScoreStatsType(const std::string& str)
{
  return ConvertCharToScoreStatsType(str.c_str());
}

inline FeatureStatsType ConvertCharToFeatureStatsType(const char *str)
{
  return static_cast<FeatureStatsType>(std::atof(str));
}

inline FeatureStatsType ConvertStringToFeatureStatsType(const std::string &str)
{
  return ConvertCharToFeatureStatsType(str.c_str());
}

// Utilities to measure decoding time
void ResetUserTime();
void PrintUserTime(const std::string &message);
double GetUserTime();

#endif  // UTIL_H
