#pragma once
#include <string>
#include "legacy/ThreadPool.h"

class System;
class Manager;

class TranslationTask : public Task
{
public:
	TranslationTask(System &system, const std::string &line);
	virtual ~TranslationTask();
	virtual void Run();

protected:
	Manager *m_mgr;
};
