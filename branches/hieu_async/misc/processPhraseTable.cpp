#include <iostream>
//#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iterator>
#include <functional>
#include <sys/stat.h>
#include "TypeDef.h"
#include "PhraseDictionaryTree.h"
#include "ConfusionNet.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "InputFileStream.h"
#include "Timer.h"

Timer timer;

template<typename T>
std::ostream& operator<<(std::ostream& out,const std::vector<T>& x)
{
	out<<x.size()<<" ";
	typename std::vector<T>::const_iterator iend=x.end();
	for(typename std::vector<T>::const_iterator i=x.begin();i!=iend;++i) 
		out<<*i<<' ';
	return out;
}

inline bool existsFile(const char* filename) {
  struct stat mystat;
  return  (stat(filename,&mystat)==0);
}
inline bool existsFile(const std::string& filename) {
  return existsFile(filename.c_str());
}

int main(int argc,char **argv) {
	std::string fto;size_t noScoreComponent=5;int cn=0;
	std::vector<std::pair<std::string,std::pair<char*,char*> > > ftts;
	int verb=0;
	for(int i=1;i<argc;++i) {
		std::string s(argv[i]);
		if(s=="-ttable") {
			std::pair<char*,char*> p;
			p.first=argv[++i];
			p.second=argv[++i];
			ftts.push_back(std::make_pair(std::string(argv[++i]),p));
		}
		else if(s=="-nscores") noScoreComponent=atoi(argv[++i]);
		else if(s=="-out") fto=std::string(argv[++i]);
		else if(s=="-cn") cn=1;
		else if(s=="-irst") cn=2;
		else if(s=="-v") verb=atoi(argv[++i]);
		else if(s=="-h") 
			{
				std::cerr<<"usage "<<argv[0]<<" :\n\n"
					"options:\n"
					"\t-ttable int int string   -- translation table file, use '-' for stdin\n"
					"\t-out string      -- output file name prefix for binary ttable\n"
					"\t-nscores int     -- number of scores in ttable\n"
					"\nfunctions:\n"
					"\t - convert ascii ttable in binary format\n"
					"\t - if ttable is not read from stdin:\n"
					"\t     treat each line as source phrase an print tgt candidates\n"
					"\n";
				return 1;
			}
		else 
			{
				std::cerr<<"ERROR: unknown option '"<<s<<"'\n";
				return 1;
			}
	}
	
	if(ftts.size()) {
			std::cerr<<"processing ptree for\n";

			if(ftts.size()==1 && ftts[0].first=="-") 
			{
				PhraseDictionaryTree pdt(noScoreComponent);
				pdt.Create(std::cin,fto);
			}
			else 
			{
				PhraseDictionaryTree pdt(noScoreComponent);
				InputFileStream fileStream(ftts[0].first);
				pdt.Create(fileStream, fto);
			}
	}
	
}
