/*
 *  Util.cpp
 *  met - Minimum Error Training
 * 
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include <stdexcept>
#include "Util.h"

#include "Timer.h"

using namespace std;

//global variable
Timer g_timer;

int verbose=0;

int verboselevel(){
  return verbose;
}

int setverboselevel(int v){
  verbose=v;
  return verbose;
}

int getNextPound(std::string &theString, std::string &substring, const std::string delimiter)
{
        unsigned int pos = 0;
        
        //skip all occurrences of delimiter
        while ( pos == 0 )
        {
                if ((pos = theString.find(delimiter)) != std::string::npos){
                        substring.assign(theString, 0, pos);
                        theString.erase(0,pos + delimiter.size());
                }
                else{
                        substring.assign(theString);
                        theString.assign("");
                }
        }
        return (pos);
};

inputfilestream::inputfilestream(const std::string &filePath)
: std::istream(0),
m_streambuf(0)
{
  //check if file is readable
  std::filebuf* fb = new std::filebuf();
  _good=(fb->open(filePath.c_str(), std::ios::in)!=NULL);
  
  if (filePath.size() > 3 &&
      filePath.substr(filePath.size() - 3, 3) == ".gz")
  {
    fb->close(); delete fb;
    m_streambuf = new gzfilebuf(filePath.c_str());  
  } else {
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

outputfilestream::outputfilestream(const std::string &filePath)
: std::ostream(0),
m_streambuf(0)
{
  //check if file is readable
  std::filebuf* fb = new std::filebuf();
  _good=(fb->open(filePath.c_str(), std::ios::out)!=NULL);  
  
  if (filePath.size() > 3 && filePath.substr(filePath.size() - 3, 3) == ".gz")
  {
	throw runtime_error("Output to a zipped file not supported!");
  } else {
    m_streambuf = fb;
  }
  this->init(m_streambuf);
}

outputfilestream::~outputfilestream()
{
  delete m_streambuf; m_streambuf = 0;
}

void outputfilestream::close()
{
}

int swapbytes(char *p, int sz, int n)
{
  char c, *l, *h;
  
  if((n<1) || (sz<2)) return 0;
  for(; n--; p+=sz) for(h=(l=p)+sz; --h>l; l++) { c=*h; *h=*l; *l=c; }
	return 0;

};

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

