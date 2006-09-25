#include <math.h>
#include <stdio.h>

#include "score.h"

int comps_n = 9;

void comps_addto(int *comps1, int *comps2) {
  int i;
  for (i=0; i<comps_n; i++)
    comps1[i] += comps2[i];
}

float compute_score(int *comps) {
  float logbleu = 0.0, brevity;
  int i;
  int n = (comps_n-1)/2;

  /*for (i=0; i<comps_n; i++)
    fprintf(stderr, " %d", comps[i]);
    fprintf(stderr, "\n");*/

  for (i=0; i<n; i++) {
    if (comps[2*i] == 0)
      return 0.0;
    logbleu += log(comps[2*i])-log(comps[2*i+1]);
  }
  logbleu /= n;
  brevity = 1.0-(float)comps[comps_n-1]/comps[1]; // comps[comps_n-1] is the ref length, comps[1] is the test length
  if (brevity < 0.0)
    logbleu += brevity;
  return exp(logbleu);
}
