#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iterator>
#include <functional>
#include <sys/stat.h>
#include "PhraseDictionaryTree.h"

inline bool existsFile(const char* filename) {
  struct stat mystat;
  return  (stat(filename,&mystat)==0);
}
inline bool existsFile(const std::string& filename) {
  return existsFile(filename.c_str());
}

int main(int argc,char **argv) {
	std::string ftt,fto;size_t noScoreComponent=5;
	for(int i=1;i<argc;++i) {
		std::string s(argv[i]);
		if(s=="-ttable") ftt=std::string(argv[++i]);
		else if(s=="-nscores") noScoreComponent=atoi(argv[++i]);
		else if(s=="-out") fto=std::string(argv[++i]);
		else if(s=="-h") 
			{
				std::cerr<<"usage "<<argv[0]<<" :\n\n"
					"options:\n"
					"\t-ttable string   -- translation table file, use '-' for stdin\n"
					"\t-out string      -- output file name prefix for binary ttable\n"
					"\n-nscores int     -- number of scores in ttable\n"
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
	
	if(ftt.size()) {
			std::cerr<<"processing ptree for '"<<ftt<<"'\n";
			PhraseDictionaryTree pdt(noScoreComponent);

			if(ftt=="-") pdt.Create(std::cin,fto);
			else 
				{
					if(!existsFile(ftt+".binphr.idx")) {
						std::cerr<<"bin ttable does not exist -> create it\n";
						std::ifstream in(ftt.c_str());
						if(fto.empty()) fto=ftt;
						pdt.Create(in,ftt);
					}
					std::cerr<<"reading bin ttable\n";
					pdt.Read(ftt); 

					std::cerr<<"processing stdin\n";
					std::string line;
					while(getline(std::cin,line)) {
						std::istringstream is(line);
						std::vector<std::string> f;
						std::copy(std::istream_iterator<std::string>(is),
											std::istream_iterator<std::string>(),
											std::back_inserter(f));

						std::cerr<<"got source phrase '"<<line<<"'\n";

						std::stringstream iostA,iostB;
						pdt.PrintTargetCandidates(f,iostA);
						
						PhraseDictionaryTree::PrefixPtr p(pdt.GetRoot());
						for(size_t i=0;i<f.size() && p;++i) 
							p=pdt.Extend(p,f[i]);
					
						if(p) {
							std::cerr<<"retrieving candidates from prefix ptr\n";
							pdt.PrintTargetCandidates(p,iostB);}
						else std::cerr<<"final Ptr invalid!\n";
						
						if(iostA.str() != iostB.str()) 
							std::cerr<<"ERROR: translation candidates mismatch '"<<iostA.str()<<"' and for prefix pointer: '"<<iostB.str()<<"'\n";
						
						std::cerr<<"translation candidates:\n"<<iostA.str()<<"\n";
						
					}
				}		
	}
	
}
