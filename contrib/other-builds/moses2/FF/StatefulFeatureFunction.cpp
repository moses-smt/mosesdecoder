/*
 * StatefulFeatureFunction.cpp
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <boost/foreach.hpp>
#include "StatefulFeatureFunction.h"
#include "../Search/Hypothesis.h"

StatefulFeatureFunction::StatefulFeatureFunction(size_t startInd, const std::string &line)
:FeatureFunction(startInd, line)
{
}

StatefulFeatureFunction::~StatefulFeatureFunction() {
	// TODO Auto-generated destructor stub
}

void StatefulFeatureFunction::EvaluateWhenApplied(const ObjectPoolContiguous<Hypothesis*> &hypos) const
{
#ifdef __linux
	pthread_t thread;
	handle = pthread_self();

    int s;
    cpu_set_t cpusetOrig, cpuset;
    s = pthread_getaffinity_np(handle, sizeof(cpu_set_t), &cpusetOrig);

    CPU_ZERO(&cpuset);

    int core = handle % 8;
    core += 24;
    CPU_SET(core, &cpuset);

    s = pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpuset);

#endif

	for (size_t i = 0; i < hypos.size(); ++i) {
		Hypothesis *hypo = hypos.get(i);
		 hypo->EvaluateWhenApplied(*this);
	 }

#ifdef __linux

    s = pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpusetOrig);
#endif
}
