#include "TranslationTask.h"
#include "Search/Manager.h"
#include "Search/Hypothesis.h"

using namespace std;

TranslationTask::TranslationTask(System &system, const std::string &line)
{
	m_mgr = new Manager(system, line);
}

TranslationTask::~TranslationTask()
{
	delete m_mgr;
}

void TranslationTask::Run()
{
	m_mgr->Decode();
	m_mgr->OutputBest(cout);
}
