/*
 *  nbest: tool to process moses n-best lists
 *
 *  File: Hypo.cpp
 *        basic functions to process one hypothesis
 *
 *  Created by Holger Schwenk, University of Le Mans, 05/16/2008
 *
 */


#include "Hypo.h"
#include <iostream>

//const char* NBEST_DELIM = "|||";

Hypo::Hypo()
{
  //cerr << "Hypo: constructor called" << endl;
}

Hypo::~Hypo()
{
  //cerr << "Hypo: destructor called" << endl;
}

void Hypo::Write(ofstream &outf)
{
  outf << id << NBEST_DELIM2 << trg << NBEST_DELIM2;
  for (vector<float>::iterator i = f.begin(); i != f.end(); i++)
    outf << (*i) << " ";
  outf << NBEST_DELIM << " " << s << endl;

}

float Hypo::CalcGlobal(Weights &w)
{
  //cerr << " HYP: calc global" << endl;
  int sz=w.val.size();
  if (sz<f.size()) {
    cerr << " - NOTE: padding weight vector with " << f.size()-sz << " zeros" << endl;
    w.val.resize(f.size());
  }

  s=0;
  for (int i=0; i<f.size(); i++) {
    //cerr << "i=" << i << ", " << w.val[i] << ", " << f[i] << endl;
    s+=w.val[i]*f[i];
  }
  //cerr << "s=" << s << endl;
  return s;
}

// this is actually a "greater than" since we want to sort in descending order
bool Hypo::operator< (const Hypo &h2) const
{
  return (this->s > h2.s);
}

