
#ifdef WIN32
#include <windows.h>
#include <io.h>
#else
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
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



inputfilestream::inputfilestream(const std::string &filePath)
: std::istream(0),
m_streambuf(0)
{
  if (filePath.size() > 3 &&
      filePath.substr(filePath.size() - 3, 3) == ".gz")
  {
    m_streambuf = new gzfilebuf(filePath.c_str());
  } else {
    std::filebuf* fb = new std::filebuf();
    _good=(fb->open(filePath.c_str(), std::ios::in)!=NULL);
    m_streambuf = fb;
  }
  this->init(m_streambuf);
}

inputfilestream::~inputfilestream()
{
  delete m_streambuf; m_streambuf = 0;
}

void inputfilestream::close()
{
}



/* MemoryMap Management
Code kindly provided by Fabio Brugnara, ITC-irst Trento.
How to use it: 
- call MMap with offset and required size (psgz):
  pg->b = MMap(fd, rdwr,offset,pgsz,&g);
- correct returned pointer with the alignment gap and save the gap:
  pg->b += pg->gap = g;
- when releasing mapped memory, subtract the gap from the pointer and add 
  the gap to the requested dimension 		
  Munmap(pg->b-pg->gap, pgsz+pg->gap, 0);
*/


void *MMap(int	fd, int	access, off_t	offset, size_t	len, off_t	*gap)
{
	void	*p;
	int	pgsz,g=0;
  
#ifdef _WIN32
/*  
// code for windows must be checked 
	HANDLE	fh,
		mh;
  
	fh = (HANDLE)_get_osfhandle(fd);
    if(offset) {
      // bisogna accertarsi che l'offset abbia la granularita`
      //corretta, MAI PROVATA! 
      SYSTEM_INFO	si;
      
      GetSystemInfo(&si);
      g = *gap = offset % si.dwPageSize;
    } else if(gap) {
      *gap=0;
    }
	if(!(mh=CreateFileMapping(fh, NULL, PAGE_READWRITE, 0, len+g, NULL))) {
		return 0;
	}
	p = (char*)MapViewOfFile(mh, FILE_MAP_ALL_ACCESS, 0,
                           offset-*gap, len+*gap);
	CloseHandle(mh);
*/
  
#else
	if(offset) {
		pgsz = sysconf(_SC_PAGESIZE);
		g = *gap = offset%pgsz;
	} else if(gap) {
		*gap=0;
	}
	p = mmap((void*)0, len+g, access,
           MAP_SHARED|MAP_FILE,
           fd, offset-g);
	if((long)p==-1L) {
		perror("mmap failed");
		p=0;
	}
#endif
	return p;
}


int Munmap(void	*p,size_t	len,int	sync)
{
	int	r=0;
  
#ifdef _WIN32
/*
  //code for windows must be checked
	if(sync) FlushViewOfFile(p, len);
	UnmapViewOfFile(p);
*/  
#else
	if(sync) msync(p, len, MS_SYNC);
	if((r=munmap((void*)p, len))) perror("munmap() failed");
#endif
	return r;
}

  
