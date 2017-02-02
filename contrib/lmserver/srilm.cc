#include <cassert>
#include <iostream>
#include "Ngram.h"

using namespace std;
Vocab vocab;
Ngram* ngram = NULL;

extern "C" {

void srilm_init(const char* fname, int order) {
  cerr << "Loading " << order << "-gram LM: " << fname << endl;
  File file(fname, "r", 0);
  assert(file);
  ngram = new Ngram(vocab, order);
  ngram->read(file, false);
  cerr << "Done\n";
}

int srilm_getvoc(const char* word) {
  return vocab.getIndex((VocabString)word);
}

float srilm_wordprob(int w, int* context) {
  return (float)ngram->wordProb(w, (VocabIndex*)context);
}

}

