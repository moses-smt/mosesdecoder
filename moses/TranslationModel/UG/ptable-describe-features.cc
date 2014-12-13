#include "mmsapt.h"
#include "moses/TranslationModel/PhraseDictionaryTreeAdaptor.h"
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/shared_ptr.hpp>
#include <algorithm>
#include <iostream>

using namespace Moses;
using namespace bitext;
using namespace std;
using namespace boost;

int main()
{
  string line;
  while(getline(cin,line))
    {
      if (line.empty()) continue;
      size_t k = line.find_first_not_of(" ");
      if (line.find("Mmsapt") != k && 
	  line.find("PhraseDictionaryBitextSampling") != k)
	continue;
      Mmsapt PT(line);
      PT.Load(false);
      cout << PT.GetName() << ":" << endl;
      vector<string> const& fnames = PT.GetFeatureNames();
      BOOST_FOREACH(string const& s, fnames)
	cout << s << endl;
      cout << endl;
    }
  exit(0);
}
  
  

