/****************************************************
 * Moses - factored phrase-based language decoder   *
 * Copyright (C) 2015 University of Edinburgh       *
 * Licensed under GNU LGPL Version 2.1, see COPYING *
 ****************************************************/

#ifndef MOSESDECODER_PERF2_BATCHEDFEATUREFUNCTION_H
#define MOSESDECODER_PERF2_BATCHEDFEATUREFUNCTION_H

#include "StatefulFeatureFunction.h"

/**
 * A feature function that can be evaluated in batches.
 * Used to implement prefetching language model queries.
 */
class BatchedFeatureFunction : public StatefulFeatureFunction {
public:
  BatchedFeatureFunction(size_t startInd, const std::string &line): StatefulFeatureFunction(startInd, line) {}

  /**
   * Evaluate a batch of Hypotheses in one go.
   */
  virtual void EvaluateWhenAppliedBatched(Hypothesis **begin, Hypothesis **end, const Manager &mgr) const = 0;

  virtual ~BatchedFeatureFunction() {}
};

#endif //MOSESDECODER_PERF2_BATCHEDFEATUREFUNCTION_H
