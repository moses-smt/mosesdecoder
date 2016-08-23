/*
 *  Util.cpp
 *  mert - Minimum Error Rate Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include "Util.h"
#include "Timer.h"

using namespace std;

namespace
{

MosesTuning::Timer g_timer;
int g_verbose = 0;

bool FindDelimiter(const std::string &str, const std::string &delim, size_t *pos)
{
  *pos = str.find(delim);
  return *pos != std::string::npos ? true : false;
}

} // namespace

namespace MosesTuning
{

int verboselevel()
{
  return g_verbose;
}

int setverboselevel(int v)
{
  g_verbose = v;
  return g_verbose;
}

size_t getNextPound(std::string &str, std::string &substr,
                    const std::string &delimiter)
{
  size_t pos = 0;

  // skip all occurrences of delimiter
  while (pos == 0) {
    if (FindDelimiter(str, delimiter, &pos)) {
      substr.assign(str, 0, pos);
      str.erase(0, pos + delimiter.size());
    } else {
      substr.assign(str);
      str.assign("");
    }
  }
  return pos;
}

void split(const std::string &s, char delim, std::vector<std::string> &elems)
{
  std::stringstream ss(s);
  std::string item;
  while(std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
}

void Tokenize(const char *str, const char delim,
              std::vector<std::string> *res)
{
  while (1) {
    const char *begin = str;
    while (*str != delim && *str) str++;
    if (begin != str)            // Don't create empty string objects.
      res->push_back(std::string(begin, str));
    if (*str++ == 0) break;
  }
}

void ResetUserTime()
{
  g_timer.start();
}

void PrintUserTime(const std::string &message)
{
  g_timer.check(message.c_str());
}

double GetUserTime()
{
  return g_timer.get_elapsed_cpu_time();
}

}
