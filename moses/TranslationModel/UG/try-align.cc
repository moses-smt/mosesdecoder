#include "mmsapt.h"
using namespace std;
using namespace Moses;

// currently broken

Mmsapt* PT;
int main(int argc, char* argv[])
{
  // string base = argv[1];
  // string L1   = argv[2];
  // string L2   = argv[3];
  // ostringstream buf;
  // buf << "Mmsapt name=PT0 output-factor=0 num-features=5 base="
  //     << base << " L1=" << L1 << " L2=" << L2;
  // string configline = buf.str();
  // PT = new Mmsapt(configline);
  // PT->Load();
  // float w[] = { 0.0582634, 0.0518865, 0.0229819, 0.00640856,  0.647506 };
  // vector<float> weights(w,w+5);
  // PT->setWeights(weights);
  // // these values are taken from a moses.ini file;
  // // is there a convenient way of accessing them from within mmsapt ???
  // string eline,fline;
  // // TokenIndex V; V.open("crp/trn/mm/de.tdx");
  // while (getline(cin,eline) && getline(cin,fline))
  //   {
  //     cout << eline << endl;
  //     cout << fline << endl;
  //     PT->align(eline,fline);
  //   }
  // delete PT;
}

