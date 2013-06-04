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

#ifdef WIN32
#include <windows.h>
#else
#include <sys/times.h>
#include <sys/resource.h>
#endif

#include <cstring>
#include <cctype>
#include <algorithm>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include "TypeDef.h"
#include "Util.h"
#include "Timer.h"
#include "util/exception.hh"
#include "util/file.hh"

using namespace std;

namespace Moses
{

//global variable
Timer g_timer;

string GetTempFolder()
{
#ifdef _WIN32
  char *tmpPath = getenv("TMP");
  string str(tmpPath);
  if (str.substr(str.size() - 1, 1) != "\\")
    str += "\\";
  return str;
#else
  return "/tmp/";
#endif
}

const std::string ToLower(const std::string& str)
{
  std::string lc(str);
  std::transform(lc.begin(), lc.end(), lc.begin(), (int(*)(int))std::tolower);
  return lc;
}

class BoolValueException : public util::Exception {};

template<>
bool Scan<bool>(const std::string &input)
{
  std::string lc = ToLower(input);
  if (lc == "yes" || lc == "y" || lc == "true" || lc == "1")
    return true;
  if (lc == "no" || lc == "n" || lc =="false" || lc == "0")
    return false;
  UTIL_THROW(BoolValueException, "Could not interpret " << input << " as a boolean.  After lowercasing, valid values are yes, y, true, 1, no, n, false, and 0.");
}

bool FileExists(const std::string& filePath)
{
  ifstream ifs(filePath.c_str());
  return !ifs.fail();
}

const std::string Trim(const std::string& str, const std::string dropChars)
{
  std::string res = str;
  res.erase(str.find_last_not_of(dropChars)+1);
  return res.erase(0, res.find_first_not_of(dropChars));
}

void ResetUserTime()
{
  g_timer.start();
};

void PrintUserTime(const std::string &message)
{
  g_timer.check(message.c_str());
}

double GetUserTime()
{
  return g_timer.get_elapsed_time();
}

std::map<std::string, std::string> ProcessAndStripSGML(std::string &line)
{
  std::map<std::string, std::string> meta;
  std::string lline = ToLower(line);
  if (lline.find("<seg")!=0) return meta;
  size_t close = lline.find(">");
  if (close == std::string::npos) return meta; // error
  size_t end = lline.find("</seg>");
  std::string seg = Trim(lline.substr(4, close-4));
  std::string text = line.substr(close+1, end - close - 1);
  for (size_t i = 1; i < seg.size(); i++) {
    if (seg[i] == '=' && seg[i-1] == ' ') {
      std::string less = seg.substr(0, i-1) + seg.substr(i);
      seg = less;
      i = 0;
      continue;
    }
    if (seg[i] == '=' && seg[i+1] == ' ') {
      std::string less = seg.substr(0, i+1);
      if (i+2 < seg.size()) less += seg.substr(i+2);
      seg = less;
      i = 0;
      continue;
    }
  }
  line = Trim(text);
  if (seg == "") return meta;
  for (size_t i = 1; i < seg.size(); i++) {
    if (seg[i] == '=') {
      std::string label = seg.substr(0, i);
      std::string val = seg.substr(i+1);
      if (val[0] == '"') {
        val = val.substr(1);
        size_t close = val.find('"');
        if (close == std::string::npos) {
          TRACE_ERR("SGML parse error: missing \"\n");
          seg = "";
          i = 0;
        } else {
          seg = val.substr(close+1);
          val = val.substr(0, close);
          i = 0;
        }
      } else {
        size_t close = val.find(' ');
        if (close == std::string::npos) {
          seg = "";
          i = 0;
        } else {
          seg = val.substr(close+1);
          val = val.substr(0, close);
        }
      }
      label = Trim(label);
      seg = Trim(seg);
      meta[label] = val;
    }
  }
  return meta;
}

std::string PassthroughSGML(std::string &line, const std::string tagName, const std::string& lbrackStr, const std::string& rbrackStr)
{
  string lbrack = lbrackStr; // = "<";
  string rbrack = rbrackStr; // = ">";

  std::string meta = "";

  std::string lline = ToLower(line);
  size_t open = lline.find(lbrack+tagName);
  //check whether the tag exists; if not return the empty string
  if (open == std::string::npos) return meta;

  size_t close = lline.find(rbrack, open);
  //check whether the tag is closed with '/>'; if not return the empty string
  if (close == std::string::npos) {
    TRACE_ERR("PassthroughSGML error: the <passthrough info/> tag does not end properly\n");
    return meta;
  }
  // extract the tag
  std::string tmp = line.substr(open, close - open + 1);
  meta = line.substr(open, close - open + 1);

  // strip the tag from the line
  line = line.substr(0, open) + line.substr(close + 1, std::string::npos);

  TRACE_ERR("The input contains a <passthrough info/> tag:" << meta << std::endl);

  lline = ToLower(line);
  open = lline.find(lbrack+tagName);
  if (open != std::string::npos) {
    TRACE_ERR("PassthroughSGML error: there are two <passthrough> tags\n");
  }
  return meta;
}

}


