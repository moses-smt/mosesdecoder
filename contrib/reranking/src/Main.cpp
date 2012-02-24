/*
 *  nbest: tool to process moses n-best lists
 *
 *  File: Main.cpp
 *        command line interface
 *
 *  Created by Holger Schwenk, University of Le Mans, 05/16/2008
 *
 */

#include <iostream>
#include <fstream>
#include "ParameterNBest.h"
#include "NBest.h"
#include "Tools.h"

#include "../../../moses/src/Util.h"


using namespace std;

int main (int argc, char *argv[])
{
  // parse parameters
  ParameterNBest *parameter = new ParameterNBest();
  if (!parameter->LoadParam(argc, argv)) {
    parameter->Explain();
    delete parameter;
    return 1;
  }

  // read input
  ifstream inpf;
  PARAM_VEC p=parameter->GetParam("input-file");
  if (p.size()<1 || p.size()>2) Error("The option -input-file requires one or two arguments");
  int in_n=p.size()>1 ? Moses::Scan<int>(p[1]) : 0;
  cout << "NBest version 0.1, written by Holger.Schwenk@lium.univ-lemans.fr" << endl
       << " - reading input from file '" << p[0] << "'";
  if (in_n>0) cout << " (limited to the first " << in_n << " hypothesis)";
  cout << endl;
  inpf.open(p[0].c_str());
  if (inpf.fail()) {
    perror ("ERROR");
    exit(1);
  }

  // open output
  ofstream outf;
  p=parameter->GetParam("output-file");
  if (p.size()<1 || p.size()>2) Error("The option -output-file requires one or two arguments");
  int out_n=p.size()>1 ? Moses::Scan<int>(p[1]) : 0;
  cout << " - writing output to file '" << p[0] << "'";
  if (out_n>0) cout << " (limited to the first " << out_n << " hypothesis)";
  cout << endl;
  outf.open(p[0].c_str());
  if (outf.fail()) {
    perror ("ERROR");
    exit(1);
  }

  // eventually read weights
  Weights w;
  int do_calc=false;
  if (parameter->isParamSpecified("weights")) {
    p=parameter->GetParam("weights");
    if (p.size()<1) Error("The option -weights requires one argument");
    cout << " - reading weights from file '" << p[0] << "'";
    int n=w.Read(p[0].c_str());
    cout << " (found " << n << " values)" << endl;
    do_calc=true;
    cout << " - recalculating global scores" << endl;
  }

  // shall we sort ?
  bool do_sort = parameter->isParamSpecified("sort");
  if (do_sort) cout << " - sorting global scores" << endl;

  // main loop
  int nb_sent=0, nb_nbest=0;
  while (!inpf.eof()) {
    NBest nbest(inpf, in_n);

    if (do_calc) nbest.CalcGlobal(w);
    if (do_sort) nbest.Sort();
    nbest.Write(outf, out_n);

    nb_sent++;
    nb_nbest+=nbest.NbNBest();
  }
  inpf.close();
  outf.close();

  // display final statistics
  cout << " - processed " << nb_nbest << " n-best hypotheses in " << nb_sent << " sentences"
       << " (average " << (float) nb_nbest/nb_sent << ")" << endl;

  return 0;
}
