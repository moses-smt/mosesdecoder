#pragma once
#include <string>
#include <boost/pool/object_pool.hpp>
#include "legacy/ThreadPool.h"
#include "Search/CubePruning/Misc.h"

class System;
class Manager;

class TranslationTask : public Task
{
public:
	mutable boost::object_pool<NSCubePruning::CubeEdge> poolCubeEdge;

	TranslationTask(System &system, const std::string &line);
	virtual ~TranslationTask();
	virtual void Run();

protected:
	Manager *m_mgr;
};
