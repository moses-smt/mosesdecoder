// $Id: data.h 1307 2007-03-14 22:22:36Z hieuhoang1972 $
#ifndef DATA_H
#define DATA_H

typedef struct {
  float *features;
  int *comps;
  float m, b; // slope and intercept, used as scratch space
} candidate_t;

typedef struct {
  candidate_t **sents;
  int sents_n, sents_max, *cands_n;
} data_t;

data_t *read_data(void);

#endif
