#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "point.h"

extern int comps_n;

data_t *read_data(void) {
  FILE *fp;
  static char buf[1000];
  char *tok, *s;
  int field;
  int sent_i, cand_i, cands_n;
  int total_cands_n;
  data_t *data;
  candidate_t *cands;
  
  data = malloc(sizeof(data_t));

  data->sents_max = 100;
  data->sents_n = 0;
  data->cands_n = malloc(data->sents_max*sizeof(int));

  total_cands_n = 0;

  fp = fopen("cands.opt", "r");
  while (fgets(buf, sizeof(buf), fp) != NULL) {
    // should we check to make sure every sentence is accounted for?
    sscanf(buf, "%d %d", &sent_i, &cands_n);
    if (sent_i >= data->sents_n)
      data->sents_n = sent_i+1;
    if (sent_i >= data->sents_max) {
      data->sents_max = (sent_i+1)*2;
      data->cands_n = realloc(data->cands_n, data->sents_max*sizeof(int));
    }
    data->cands_n[sent_i] = cands_n;
    total_cands_n += cands_n;
  }
  fclose(fp);

  /* create master array for candidates and then set data->sents
     to point into it */
  cands = malloc(total_cands_n * sizeof(candidate_t));
  data->sents = malloc(data->sents_n * sizeof(candidate_t *));
  total_cands_n = 0;
  for (sent_i=0; sent_i<data->sents_n; sent_i++) {
    data->sents[sent_i] = cands+total_cands_n;
    total_cands_n += data->cands_n[sent_i];
  }


  cand_i = 0;
  fp = fopen("feats.opt", "r");
  while (fgets(buf, sizeof(buf), fp) != NULL) {
    cands[cand_i].features = malloc(dim*sizeof(float));
    cands[cand_i].comps = malloc(comps_n*sizeof(int));

    field = 0;
    s = buf;
    while ((tok = strsep(&s, " \t\n")) != NULL) {
      if (!*tok) // empty token
	continue;
      // read dim floats and then comps_n ints
      if (field < dim)
	cands[cand_i].features[field] = -strtod(tok, NULL); // Venugopal format uses costs
      else if (field < dim+comps_n)
	cands[cand_i].comps[field-dim] = strtol(tok, NULL, 10);
      else {
	fprintf(stderr, "read_data(): too many fields in line in feats.opt\n");
	return NULL;
      }
      field++;
    }
    if (field != dim+comps_n) {
      fprintf(stderr, "read_data(): wrong number of fields in line in feats.opt\n");
      return NULL;
    }
    cand_i++;
  }

  if (cand_i != total_cands_n) {
    fprintf(stderr, "read_data(): wrong number of lines in cands.opt\n");
    return NULL;
  }

  fclose(fp);

  return data;
}

