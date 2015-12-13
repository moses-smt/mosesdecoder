// $Id$
#ifndef GENERATETUPLES_H_
#define GENERATETUPLES_H_
#include "moses/PhraseDictionaryTree.h"

class ConfusionNet;

void GenerateCandidates(const ConfusionNet& src,
                        const std::vector<PhraseDictionaryTree const*>& pdicts,
                        const std::vector<std::vector<float> >& weights,
                        int verbose=0) ;
#endif
