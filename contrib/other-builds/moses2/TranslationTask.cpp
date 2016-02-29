#include "TranslationTask.h"
#include "System.h"
#include "PhraseBased/Manager.h"
#include "SCFG/Manager.h"

using namespace std;

namespace Moses2
{

TranslationTask::TranslationTask(System &system, const std::string &line, long translationId)
{
	if (system.searchAlgorithm == CYKPlus) {
		m_mgr = new SCFG::Manager(system, *this, line, translationId);
	}
	else {
		m_mgr = new Manager(system, *this, line, translationId);
	}
}

TranslationTask::~TranslationTask()
{
	delete m_mgr;
}

void TranslationTask::Run()
{
	m_mgr->Decode();
}

}

