#include "TranslationTask.h"
#include "Search/Manager.h"
#include "Search/Hypothesis.h"

using namespace std;

namespace Moses2
{

TranslationTask::TranslationTask(System &system, const std::string &line)
{
	m_mgr = new Manager(system, *this, line);
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

}

