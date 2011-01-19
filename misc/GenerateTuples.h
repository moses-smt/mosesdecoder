// $Id: GenerateTuples.h 359 2006-07-28 18:14:20Z zens $
#ifndef GENERATETUPLES_H_
#define GENERATETUPLES_H_
#include "PhraseDictionaryTree.h"

class ConfusionNet;

void GenerateCandidates(const ConfusionNet& src,
												const std::vector<PhraseDictionaryTree const*>& pdicts,
												const std::vector<std::vector<float> >& weights,
												int verbose=0) ;
#endif
