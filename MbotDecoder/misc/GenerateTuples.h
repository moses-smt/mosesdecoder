// $Id: GenerateTuples.h,v 1.1.1.1 2013/01/06 16:54:14 braunefe Exp $
#ifndef GENERATETUPLES_H_
#define GENERATETUPLES_H_
#include "PhraseDictionaryTree.h"

class ConfusionNet;

void GenerateCandidates(const ConfusionNet& src,
                        const std::vector<PhraseDictionaryTree const*>& pdicts,
                        const std::vector<std::vector<float> >& weights,
                        int verbose=0) ;
#endif
