#ifndef SRILM_H_
#define SRILM_H_

void srilm_init(const char* fname, int order);
int srilm_getvoc(const char* word);
float srilm_wordprob(int, int*);

#endif
