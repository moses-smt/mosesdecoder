#include <typeinfo>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <vector>
#include "FF.h"
#include "LM.h"
#include "RO.h"
#include "PT.h"
#include "WP.h"
#include "Distortion.h"

using namespace std;

string iniPath;
vector<FF*> ffVec;
bool isHierarchical = false;

void OutputIni();

int main(int argc, char **argv)
{
  FF *model;
  for (int i = 0; i < argc; ++i) {
    string key(argv[i]);
    
    if (key == "-phrase-translation-table") {
      ++i;
      model = new PT(argv[i]);
      ffVec.push_back(model);
    }
    else if (key == "-glue-grammar-file") {
      ++i;
      model = new PT(argv[i], 1);
      ffVec.push_back(model);
    }
    else if (key == "-reordering-table") {
      ++i;
      model = new RO(argv[i]);
      ffVec.push_back(model);
    }
    else if (key == "-lm") {
      ++i;
      model = new LM(argv[i]);
      ffVec.push_back(model);
    }
    else if (key == "-config") {
      ++i;
      iniPath = argv[i];
    }
    else if (key == "-hierarchical") {
      isHierarchical = true;
    }
  }

  if (!isHierarchical) {
	  model = new WP("");
	  ffVec.insert(ffVec.begin(), model);	

	  model = new Distortion("");
	  ffVec.insert(ffVec.begin(), model);	

  }

  OutputIni();
}


// output ini file, with features and weights, and everything else
void OutputIni()
{
  ofstream strme(iniPath.c_str());
  stringstream weightStrme;

  weightStrme << "\n\n[weight]" << endl;

  strme << "[input-factors]" << endl;
  strme << "0" << endl;

  strme << "[mapping]" << endl;
  if (isHierarchical) {
    strme << "0 T 0" << endl
          << "1 T 1" << endl;
  }
  else {
    strme << "0 T 0" << endl;
  }

  if (!isHierarchical) {
    strme << "[distortion-limit]" << endl;
    strme << "6" << endl;
  }
  else {
    strme << "[cube-pruning-pop-limit]" << endl;
    strme << "1000" << endl;

    strme << "[non-terminals]" << endl;
    strme << "X" << endl;

    strme << "[search-algorithm]" << endl;
    strme << "3" << endl;

  }

  strme << "\n\n[feature]" << endl;
  for (size_t i = 0; i < ffVec.size(); ++i) {
    const FF &ff = *ffVec[i];

    ff.Output(strme);
    ff.OutputWeights(weightStrme);
  }

  strme << weightStrme.str();

  strme.close();
}


