#ifndef lmserver_srilm_h
#define lmserver_srilm_h

void srilm_init(const char* fname, int order);
int srilm_getvoc(const char* word);
float srilm_wordprob(int, int*);

#endif
