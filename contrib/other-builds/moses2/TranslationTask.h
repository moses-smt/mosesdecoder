#pragma once
#include <string>
#include "legacy/ThreadPool.h"
#include "Search/CubePruning/Misc.h"

class System;
class Manager;

class TranslationTask : public Task
{
public:
	mutable NSCubePruning::CubeEdge::Queue queue;
	mutable NSCubePruning::CubeEdge::SeenPositions seenPositions;

	TranslationTask(System &system, const std::string &line);
	virtual ~TranslationTask();
	virtual void Run();

protected:
	Manager *m_mgr;
};
