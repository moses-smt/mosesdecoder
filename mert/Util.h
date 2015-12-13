/*
 *  Util.h
 *  mert - Minimum Error Rate Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef MERT_UTIL_H_
#define MERT_UTIL_H_

#include <cmath>
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

namespace MosesTuning
{

#ifdef TRACE_ENABLE
#define TRACE_ERR(str) { std::cerr << str; }
#else
#define TRACE_ERR(str) { }
#endif

#if __GNUC__ == 4 && __GNUC_MINOR__ == 8 && (__GNUC_PATCHLEVEL__ == 1 || __GNUC_PATCHLEVEL__ == 2)
// gcc nth_element() bug
#define NTH_ELEMENT3(begin, middle, end) std::sort(begin, end)
#define NTH_ELEMENT4(begin, middle, end, orderer) std::sort(begin, end, orderer)
#else
#define NTH_ELEMENT3(begin, middle, end) std::nth_element(begin, middle, end)
#define NTH_ELEMENT4(begin, middle, end, orderer) std::nth_element(begin, middle, end, orderer)
#endif

const char kDefaultDelimiterSymbol[] = " ";

int verboselevel();
int setverboselevel(int v);


const float kEPS = 0.0001f;

template <typename T>
bool IsAlmostEqual(T expected, T actual, float round=kEPS)
{
  if (std::abs(expected - actual) < round) {
    return true;
  } else {
    std::cerr << "Fail: expected = " << expected
              << " (actual = " << actual << ")" << std::endl;
    return false;
  }
}

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

/**
 * Split the string 'str' with specified delimitter 'delim' into tokens.
 * The resulting tokens are set to 'res'.
 *
 * ex. "a,b,c" => {"a", "b", "c"}.
 */
void Tokenize(const char *str, const char delim, std::vector<std::string> *res);

template<typename T>
inline T Scan(const std::string &input)
{
  std::stringstream stream(input);
  T ret;
  stream >> ret;
  return ret;
}

/**
 * Returns true iff "str" ends with "suffix".
 * e.g., Given str = "abc:" and suffix = ":", this function returns true.
 */
inline bool EndsWith(const std::string& str, const char* suffix)
{
  return str.find_last_of(suffix) == str.size() - 1;
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

inline std::string trimStr(const std::string& Src, const std::string& c = " \r\n")
{
  size_t p2 = Src.find_last_not_of(c);
  if (p2 == std::string::npos) return std::string();
  size_t p1 = Src.find_first_not_of(c);
  if (p1 == std::string::npos) p1 = 0;
  return Src.substr(p1, (p2-p1)+1);
}

// Utilities to measure decoding time
void ResetUserTime();
void PrintUserTime(const std::string &message);
double GetUserTime();

}

#endif  // MERT_UTIL_H_
