/*
 *  nbest: tool to process moses n-best lists
 *
 *  File: Tools.cpp
 *        basic utility functions
 *
 *  Created by Holger Schwenk, University of Le Mans, 05/16/2008
 *
 */

#include "Tools.h"

int Weights::Read(const char *fname)
{
  ifstream inpf;

  inpf.open(fname);
  if (inpf.fail()) {
    perror ("ERROR");
    exit(1);
  }

  float f;
  while (inpf >> f) val.push_back(f);

  inpf.close();
  return val.size();
}

