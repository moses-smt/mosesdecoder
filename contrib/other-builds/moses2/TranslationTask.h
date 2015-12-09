#pragma once
#include <boost/pool/object_pool.hpp>
#include <string>
#include "legacy/ThreadPool.h"
#include "Search/CubePruning/Misc.h"
#include "Search/Hypothesis.h"

class System;
class Manager;

class TranslationTask : public Task
{
public:
	mutable NSCubePruning::CubeEdge::Queue queue;
	mutable NSCubePruning::CubeEdge::SeenPositions seenPositions;
	mutable boost::object_pool<Hypothesis> hypoPool;

	TranslationTask(System &system, const std::string &line);
	virtual ~TranslationTask();
	virtual void Run();

protected:
	Manager *m_mgr;
};
