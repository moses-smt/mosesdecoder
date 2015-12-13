#include <iostream>
//#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iterator>
#include <functional>
#include <sys/stat.h>
#include "moses/TypeDef.h"
#include "moses/ConfusionNet.h"
#include "moses/FactorCollection.h"
#include "moses/Phrase.h"
#include "moses/InputFileStream.h"
#include "moses/Timer.h"
#include "moses/TranslationModel/PhraseDictionaryTree.h"

using namespace std;
using namespace Moses;

Timer timer;

template<typename T>
std::ostream& operator<<(std::ostream& out,const std::vector<T>& x)
{
  out<<x.size()<<" ";
  typename std::vector<T>::const_iterator iend=x.end();
  for(typename std::vector<T>::const_iterator i=x.begin(); i!=iend; ++i)
    out<<*i<<' ';
  return out;
}

inline bool existsFile(const char* filename)
{
  struct stat mystat;
  return  (stat(filename,&mystat)==0);
}
inline bool existsFile(const std::string& filename)
{
  return existsFile(filename.c_str());
}

int main(int argc,char **argv)
{
  std::string fto;
  size_t noScoreComponent=5;
  int cn=0;
  bool aligninfo=true;
  std::vector<std::pair<std::string,std::pair<char*,char*> > > ftts;
  int verb=0;
  for(int i=1; i<argc; ++i) {
    std::string s(argv[i]);
    if(s=="-ttable") {
      std::pair<char*,char*> p;
      p.first=argv[++i];
      p.second=argv[++i];
      ftts.push_back(std::make_pair(std::string(argv[++i]),p));
    } else if(s=="-nscores") noScoreComponent=atoi(argv[++i]);
    else if(s=="-out") fto=std::string(argv[++i]);
    else if(s=="-cn") cn=1;
    else if(s=="-irst") cn=2;
    else if(s=="-no-alignment-info") aligninfo=false;
    else if(s=="-v") verb=atoi(argv[++i]);
    else if(s=="-h") {
      std::cerr<<"usage "<<argv[0]<<" :\n\n"
               "options:\n"
               "\t-ttable int int string   -- translation table file, use '-' for stdin\n"
               "\t-out string      -- output file name prefix for binary ttable\n"
               "\t-nscores int     -- number of scores in ttable\n"
               "\t-no-alignment-info  -- omit alignment info from the binary ttable \n"
               "\nfunctions:\n"
               "\t - convert ascii ttable in binary format\n"
               "\t - if ttable is not read from stdin:\n"
               "\t     treat each line as source phrase an print tgt candidates\n"
               "\n";
      return 1;
    } else {
      std::cerr<<"ERROR: unknown option '"<<s<<"'\n";
      return 1;
    }
  }


  if(ftts.size()) {

    if(ftts.size()==1) {
      std::cerr<<"processing ptree for ";
      PhraseDictionaryTree pdt;

      pdt.PrintWordAlignment(aligninfo);

      if (ftts[0].first=="-") {
        std::cerr<< "stdin\n";
        pdt.Create(std::cin,fto);
      } else {
        std::cerr<< ftts[0].first << "\n";
        InputFileStream in(ftts[0].first);
        pdt.Create(in,fto);
      }
    } else {
#if 0
      std::vector<PhraseDictionaryTree const*> pdicts;
      std::vector<FactorType> factorOrder;
      for(size_t i=0; i<ftts.size(); ++i) {

        PhraseDictionaryTree *pdtptr=new PhraseDictionaryTree(noScoreComponent,
            &factorCollection,
            getFactorType(atoi(ftts[i].second.first)),
            getFactorType(atoi(ftts[i].second.second))
                                                             );
        factorOrder.push_back(pdtptr->GetInputFactorType());
        PhraseDictionaryTree &pdt=*pdtptr;
        pdicts.push_back(pdtptr);

        std::string facStr="."+std::string(ftts[i].second.first)+"-"+std::string(ftts[i].second.second);
        std::string prefix=ftts[i].first+facStr;
        if(!existsFile(prefix+".binphr.idx")) {
          std::cerr<<"bin ttable does not exist -> create it\n";
          InputFileStream in(prefix);
          pdt.Create(in,prefix);
        }
        std::cerr<<"reading bin ttable\n";
        pdt.Read(prefix);

      }

      std::cerr<<"processing stdin\n";
      if(!cn) {
        std::string line;
        while(getline(std::cin,line)) {
          std::istringstream is(line);
#if 0
          std::vector<std::string> f;
          std::copy(std::istream_iterator<std::string>(is),
                    std::istream_iterator<std::string>(),
                    std::back_inserter(f));
#endif
          std::cerr<<"got source phrase '"<<line<<"'\n";

          Phrase F(Input);
          F.CreateFromString(factorOrder,line,factorCollection);

          for(size_t k=0; k<pdicts.size(); ++k) {
            PhraseDictionaryTree const& pdt=*pdicts[k];

            std::vector<std::string> f(F.GetSize());
            for(size_t i=0; i<F.GetSize(); ++i)
              f[i]=F.GetFactor(i,pdt.GetInputFactorType())->ToString();

            std::stringstream iostA,iostB;
            std::cerr<<"full phrase processing "<<f<<"\n";
            pdt.PrintTargetCandidates(f,iostA);

            std::cerr<<"processing with prefix ptr\n";
            PhraseDictionaryTree::PrefixPtr p(pdt.GetRoot());

            for(size_t i=0; i<f.size() && p; ++i) {
              std::cerr<<"pre "<<i<<" "<<(p?"1":"0")<<"\n";
              p=pdt.Extend(p,f[i]);
              std::cerr<<"post "<<i<<" "<<(p?"1":"0")<<"\n";
            }
            if(p) {
              std::cerr<<"retrieving candidates from prefix ptr\n";
              pdt.PrintTargetCandidates(p,iostB);
            } else {
              std::cerr<<"final ptr is invalid\n";
              iostB<<"there are 0 target candidates\n";
            }
            if(iostA.str() != iostB.str())
              std::cerr<<"ERROR: translation candidates mismatch '"<<iostA.str()<<"' and for prefix pointer: '"<<iostB.str()<<"'\n";

            std::cerr<<"translation candidates:\n"<<iostA.str()<<"\n";
            pdt.FreeMemory();

          }

        }
      } else {
        // process confusion net input
        ConfusionNet net(&factorCollection);
        std::vector<std::vector<float> > weights;
        for(size_t i=0; i<pdicts.size(); ++i)
          weights.push_back(std::vector<float>(noScoreComponent,1/(1.0*noScoreComponent)));

        while(net.ReadF(std::cin,factorOrder,cn-1)) {
          net.Print(std::cerr);
          GenerateCandidates(net,pdicts,weights,verb);
        }

      }
#else
      std::cerr<<"ERROR: these functions are currently broken...\n";
      exit(1);
#endif
    }
  }

}
