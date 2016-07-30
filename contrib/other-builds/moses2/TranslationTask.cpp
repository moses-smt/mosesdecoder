#include "TranslationTask.h"
#include "System.h"
#include "PhraseBased/Manager.h"
#include "SCFG/Manager.h"

using namespace std;

namespace Moses2
{

TranslationTask::TranslationTask(System &system,
		const std::string &line,
		long translationId)
:m_system(system)
,m_line(line)
,m_translationId(translationId)
{
}

TranslationTask::~TranslationTask()
{
}

void TranslationTask::Run()
{
  ManagerBase *mgr = GetManager();

  mgr->Decode();

  string out;

  out = mgr->OutputBest() + "\n";
  m_system.bestCollector->Write(m_translationId, out);

  if (m_system.options.nbest.nbest_size) {
    out = mgr->OutputNBest();
    m_system.nbestCollector->Write(m_translationId, out);
  }

  delete mgr;
}

ManagerBase *TranslationTask::GetManager()
{
  ManagerBase *mgr;
  if (m_system.isPb) {
	  mgr = new Manager(m_system, *this, m_line, m_translationId);
  }
  else {
	  mgr = new SCFG::Manager(m_system, *this, m_line, m_translationId);
  }
  return mgr;
}

}

