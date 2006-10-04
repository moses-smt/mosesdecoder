
#ifdef WIN32
#include <windows.h>
#endif

#include "util.h"

using namespace std;

string gettempfolder()
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


void createtempfile(ofstream  &fileStream, string &filePath, std::ios_base::openmode flags)
{	
#ifdef _WIN32
	char buffer[BUFSIZ];
	::GetTempFileNameA(gettempfolder().c_str(), "", 0, buffer);
	filePath = buffer;
#else
	char buffer[L_tmpnam];
	strcpy(buffer, gettempfolder().c_str());
	strcat(buffer, "dskbuff--XXXXXX");
	mkstemp(buffer);
	filePath = buffer;
#endif
	fileStream.open(filePath.c_str(), flags);
}

void removefile(const std::string &filePath)
{
#ifdef _WIN32
	::DeleteFileA(filePath.c_str());
#else
  char cmd[100];
  sprintf(cmd,"rm %s",filePath.c_str());
  system(cmd);
#endif
}

