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

#include <cctype>
#include <algorithm>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include "TypeDef.h"
#include "Util.h"
#include "md5.h"

using namespace std;

//global variable
unsigned long _util_usertime_sec=0;

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

void CreateTempFile(ofstream  &fileStream, string &filePath)
{	
#ifdef _WIN32
	char buffer[BUFSIZ];
	::GetTempFileNameA(GetTempFolder().c_str(), "", 0, buffer);
	filePath = buffer;
#else
	char buffer[L_tmpnam];
	strcpy(buffer, GetTempFolder().c_str());
	strcat(buffer, PROJECT_NAME);
	strcat(buffer, "--XXXXXX");
	mkstemp(buffer);
	filePath = buffer;
#endif
	fileStream.open(filePath.c_str(), ofstream::out | ofstream::app);
}

string GetMD5Hash(const string &filePath)
{
	unsigned char buffer[16384], signature[16];
	struct MD5Context md5c;

	FILE *in;
	if ((in = fopen(filePath.c_str(), "rb")) == NULL) 
	{
	  return "";
	}

	MD5Init(&md5c);
	size_t j;
  while ((j = fread(buffer, 1, sizeof buffer, in)) > 0) 
	{
		MD5Update(&md5c, buffer, (unsigned) j);
  }
	MD5Final(signature, &md5c);

	std::stringstream stream("");
	stream.setf(std::ios_base::hex,std::ios_base::basefield);
	for (j = 0; j < sizeof signature; j++) 
	{
		stream << setw(2) << setfill('0') << (int) signature[j];
  }

	return stream.str();
}

const std::string ToLower(const std::string& str)
{
	std::string lc(str);
	std::transform(lc.begin(), lc.end(), lc.begin(), (int(*)(int))std::tolower);
	return lc;
}

template<>
bool Scan<bool>(const std::string &input)
{
	std::string lc = ToLower(input);
	if (lc == "yes" || lc == "y" || lc == "true" || lc == "1")
		return true;
	if (lc == "no" || lc == "n" || lc =="false" || lc == "0")
		return false;
	std::cerr << "Scan<bool>: didn't understand '" << lc << "', returning false" << std::endl;
	return false;
}

bool FileExists(const std::string& filePath)
{
        std::ifstream ifs(filePath.c_str());
        return !ifs.fail();
}

const std::string Trim(const std::string& str, const std::string dropChars)
{
	std::string res = str;
	res.erase(str.find_last_not_of(dropChars)+1);
	return res.erase(0, res.find_first_not_of(dropChars));
}

#ifndef WIN32
void ResetUserTime()
{
  struct rusage ru;  
  if (getrusage(RUSAGE_SELF, &ru)!=-1)      
    _util_usertime_sec=(unsigned long) ru.ru_utime.tv_sec;
  else
    std::cerr << "ResetUserTime: failure in getrusage!\n";        
};

void PrintUserTime(std::ostream &out, const std::string &message){ 
  struct rusage ru;
  if (getrusage(RUSAGE_SELF, &ru)!=-1)        
    out << message.c_str() << " part(sec)=" <<  (unsigned long)ru.ru_utime.tv_sec - _util_usertime_sec 
        << " cum(sec)=" << (unsigned long)ru.ru_utime.tv_sec << "\n";    
  else{    
    std::cerr << "PrintUserTime: failure in getrusage!\n";    
    out << "PrintUserTime: failure in getrusage!\n";    
  }
}
#endif

