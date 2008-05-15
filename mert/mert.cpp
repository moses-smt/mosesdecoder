
#include <limits>
#include "Data.h"
#include "Point.h"
#include "Scorer.h"
#include "ScoreData.h"
#include "FeatureData.h"
#include "Optimizer.h"

int verbose = 2;

float min_interval = 1e-3;

void usage(void) {
  fprintf(stderr, "usage: mert -d <dimensions>\n");
  exit(1);
}

int main (int argc, char **argv) {
  int i, c,dim;


  while ((c = getopt(argc, argv, "d:n:")) != -1) {
    switch (c) {
    case 'd':
      dim = strtol(optarg, NULL, 10);
      break;
    case 'n':
      //TODO
      break;
    default:
      usage();
    }
  }
  argc -= optind;
  argv += optind;

  if (dim < 0)
    usage();


}
