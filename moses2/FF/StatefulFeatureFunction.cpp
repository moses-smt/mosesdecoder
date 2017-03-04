/*
 * StatefulFeatureFunction.cpp
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */
#ifdef __linux
#include <pthread.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <boost/foreach.hpp>
#include "StatefulFeatureFunction.h"
#include "../PhraseBased/Hypothesis.h"

using namespace std;

namespace Moses2
{

StatefulFeatureFunction::StatefulFeatureFunction(size_t startInd,
    const std::string &line) :
  FeatureFunction(startInd, line)
{
}

StatefulFeatureFunction::~StatefulFeatureFunction()
{
  // TODO Auto-generated destructor stub
}

void StatefulFeatureFunction::EvaluateWhenAppliedBatch(
  const System &system,
  const Batch &batch) const
{
  //cerr << "EvaluateWhenAppliedBatch:" << m_name << endl;
#ifdef __linux
  /*
   pthread_t handle;
   handle = pthread_self();

   int s;
   cpu_set_t cpusetOrig, cpuset;
   s = pthread_getaffinity_np(handle, sizeof(cpu_set_t), &cpusetOrig);

   CPU_ZERO(&cpuset);

   int core = handle % 8;
   core += 24;
   CPU_SET(core, &cpuset);

   s = pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpuset);
   */
#endif

  for (size_t i = 0; i < batch.size(); ++i) {
    Hypothesis *hypo = batch[i];
    hypo->EvaluateWhenApplied(*this);
  }

#ifdef __linux
  //    s = pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpusetOrig);
#endif
}

}

