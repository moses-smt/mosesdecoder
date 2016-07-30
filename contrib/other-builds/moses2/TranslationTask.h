#pragma once
#include <string>
#include "legacy/ThreadPool.h"

namespace Moses2
{

class System;
class ManagerBase;
class Manager;

class TranslationTask: public Task
{
public:

  TranslationTask(System &system, const std::string &line, long translationId);
  virtual ~TranslationTask();
  virtual void Run();

protected:
  System &m_system;
  const std::string m_line;
  const long m_translationId;

  ManagerBase *GetManager();
};

}

