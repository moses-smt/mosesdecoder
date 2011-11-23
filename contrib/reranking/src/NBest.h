/*
 *  nbest: tool to process moses n-best lists
 *
 *  File: NBest.h
 *        basic functions on n-best lists
 *
 *  Created by Holger Schwenk, University of Le Mans, 05/16/2008
 *
 */


#ifndef _NBEST_H_
#define _NBEST_H_

using namespace std;

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "Tools.h"
#include "Hypo.h"

class NBest
{
  int 		   id;
  string           src;
  vector<Hypo> nbest;
  bool ParseLine(ifstream &inpf, const int n);
public:
  NBest(ifstream&, const int=0);
  ~NBest();
  int NbNBest() {
    return nbest.size();
  };
  float CalcGlobal(Weights&);
  void Sort(); // largest values first
  void Write(ofstream&, int=0);
};

void Error(char *msg);

#endif
