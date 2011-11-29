/*
 *  nbest: tool to process moses n-best lists
 *
 *  File: Hypo.h
 *        basic functions to process one hypothesis
 *
 *  Created by Holger Schwenk, University of Le Mans, 05/16/2008
 *
 */


#ifndef _HYPO_H_
#define _HYPO_H_

using namespace std;

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "Tools.h"

#define NBEST_DELIM "|||"
#define NBEST_DELIM2 " ||| "

class Hypo
{
  int id;
  string trg;  // translation
  vector<float> f;  // feature function scores
  float	s;  // global score
  // segmentation
public:
  Hypo();
  Hypo(int p_id,string &p_trg, vector<float> &p_f, float p_s) : id(p_id),trg(p_trg),f(p_f),s(p_s) {};
  ~Hypo();
  float CalcGlobal(Weights&);
  void Write(ofstream&);
  bool operator< (const Hypo&) const;
  // bool CompareLikelihoods (const Hypo&, const Hypo&) const;
};

#endif
