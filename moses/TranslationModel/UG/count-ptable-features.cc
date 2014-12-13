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
  getline(cin,line);
  Mmsapt PT(line);
  PT.Load(false);
  cout << PT.GetFeatureNames().size() << endl;
  exit(0);
}
  
  

