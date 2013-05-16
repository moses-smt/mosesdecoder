#include <typeinfo>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <vector>
#include <stdlib.h>
#include "FF.h"
#include "LM.h"
#include "RO.h"
#include "PT.h"
#include "WP.h"
#include "UnknownWP.h"
#include "Distortion.h"

using namespace std;

string iniPath;
vector<FF*> ffVec;
vector<string> additionalIni;
bool isHierarchical = false;
int inputFactorMax = 0;

void OutputIni();
void OutputAdditionalIni(ofstream &out);
void OutputAdditionalIni(ofstream &out, const string &path);
void ParseFactors(const string &line, vector< pair<Factors, Factors> > &ret);

int main(int argc, char **argv)
{
  vector< pair<Factors, Factors> > transFactors, roFactors;

  int indTrans = 0, indRO = 0;
  FF *model;
  for (int i = 1; i < argc; ++i) {
    string key(argv[i]);
    
    if (key == "-phrase-translation-table") {
      ++i;

      pair<Factors, Factors> *factors = transFactors.size() > indTrans ? &transFactors[indTrans] : NULL;
      model = new PT(argv[i], 5, isHierarchical, factors);
      ffVec.push_back(model);

      ++indTrans;
    }
    else if (key == "-glue-grammar-file") {
      ++i;
      pair<Factors, Factors> *factors = transFactors.size() > indTrans ? &transFactors[indTrans] : NULL; 
      model = new PT(argv[i], 1, isHierarchical, factors);
      ffVec.push_back(model);

      ++indTrans;
    }
    else if (key == "-reordering-table") {
      ++i;
      pair<Factors, Factors> *factors = roFactors.size() > indRO ? &roFactors[indRO] : NULL; 
      model = new RO(argv[i], factors);
      ffVec.push_back(model);

      ++indRO;
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
    else if (key == "-translation-factors") {
      ++i;
      ParseFactors(argv[i], transFactors);
    }
    else if (key == "-reordering-factors") {
      ++i;
      ParseFactors(argv[i], roFactors);
    }
    else if (key == "-input-factor-max") {
      ++i;
      inputFactorMax = Scan<int>(argv[i]);
    }
    else if (key == "-additional-ini-file") {
      ++i;
      additionalIni.push_back(argv[i]);
    }
    else if (key == "-sparse-translation-table") {
      // ignore. TODO check that all pt can handle sparse features
    }
    else if (key == "-score-options") {
      // ignore. TODO 
      ++i;
    }
    else if (key == "-additional-ini") {
      // ignore. TODO 
      ++i;
    }

    else {
      cerr << "Unknown arg " << key << endl;
      abort();
    }
  }

  model = new WP("");
  ffVec.insert(ffVec.begin(), model);	
  model = new UnknownWP("");
  ffVec.insert(ffVec.begin(), model);	

  if (!isHierarchical) {
	  model = new Distortion("");
	  ffVec.insert(ffVec.begin(), model);	

  }

  OutputIni();
}

// parse input & output factors for 
// -translation-factors & -reordering-factors
void ParseFactorsPair(const string &line, vector< pair<Factors, Factors> > &ret)
{
  vector<string> toks = Tokenize(line, "-");
  assert(toks.size() == 2);

  Factors input, output;
  input = Tokenize<int>(toks[0], ",");
  output = Tokenize<int>(toks[1], ",");

  ret.push_back(pair<Factors, Factors>(input, output) );
}

void ParseFactors(const string &line, vector< pair<Factors, Factors> > &ret)
{
  vector<string> toks = Tokenize(line, ":");
  for (size_t i = 0; i < toks.size(); ++i) {
    const string &tok = toks[i];
    ParseFactorsPair(tok, ret);
  }
}

// output ini file, with features and weights, and everything else
void OutputIni()
{
  ofstream strme(iniPath.c_str());
  stringstream weightStrme;

  weightStrme << "\n\n[weight]" << endl;

  strme << "[input-factors]" << endl;
  for (size_t i = 0; i <= inputFactorMax; ++i) {
    strme << i << endl;
  }
  strme << endl;

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

    strme << "[max-chart-span]" << endl;
    strme << "20" << endl;
    strme << "1000" << endl;

  }

  strme << "\n\n[feature]" << endl;
  for (size_t i = 0; i < ffVec.size(); ++i) {
    const FF &ff = *ffVec[i];

    ff.Output(strme);
    ff.OutputWeights(weightStrme);
  }

  strme << weightStrme.str();
  strme << endl;

  OutputAdditionalIni(strme);

  strme << endl << endl << endl;
  strme.close();
}


void OutputAdditionalIni(ofstream &out)
{
  for (size_t i = 0; i < additionalIni.size(); ++i) {
    string &path = additionalIni[i];
    OutputAdditionalIni(out, path);
  }
}

void OutputAdditionalIni(ofstream &out, const string &path)
{
  ifstream in(path.c_str());

  string line;
  while (getline(in, line)) {
    out << line << endl;
  }
  in.close();
}


