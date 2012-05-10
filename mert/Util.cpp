/*
 *  Util.cpp
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include "Util.h"
#include "Timer.h"

using namespace std;

// global variables
Timer g_timer;

int verbose = 0;

namespace {

bool FindDelimiter(const std::string &str, const std::string &delim, size_t *pos)
{
  *pos = str.find(delim);
  return *pos != std::string::npos ? true : false;
}
} // namespace

int verboselevel()
{
  return verbose;
}

int setverboselevel(int v)
{
  verbose = v;
  return verbose;
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

void split(const std::string &s, char delim, std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while(std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
}

void Tokenize(const char *str, const char delim,
              std::vector<std::string> *res) {
  while (1) {
    const char *begin = str;
    while (*str != delim && *str) str++;
    res->push_back(std::string(begin, str));
    if (*str++ == 0) break;
  }
}

int swapbytes(char *p, int sz, int n)
{
  char c, *l, *h;

  if((n < 1) || (sz < 2)) return 0;
  for (; n--; p += sz) {
    for (h = (l = p) + sz; --h > l; l++) {
      c = *h;
      *h = *l;
      *l = c;
    }
  }
  return 0;

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
  return g_timer.get_elapsed_time();
}
