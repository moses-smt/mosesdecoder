#include "mmsapt.h"
#include "moses/TranslationModel/PhraseDictionaryTreeAdaptor.h"
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/shared_ptr.hpp>
#include <algorithm>
#include <iostream>

using namespace Moses;
using namespace sapt;
using namespace std;
using namespace boost;

int main()
{
  string line;
  getline(cin,line);
  Mmsapt PT(line);
  AllOptions::ptr opts(new AllOptions);
  PT.Load(opts, false);
  cout << PT.GetFeatureNames().size() << endl;
  exit(0);
}



