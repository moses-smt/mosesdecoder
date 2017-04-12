#include "TranslationTask.h"
#include "System.h"
#include "InputType.h"
#include "PhraseBased/Manager.h"
#include "SCFG/Manager.h"

using namespace std;

namespace Moses2
{

TranslationTask::TranslationTask(System &system,
                                 const std::string &line,
                                 long translationId)
{
  if (system.isPb) {
    m_mgr = new Manager(system, *this, line, translationId);
  } else {
    m_mgr = new SCFG::Manager(system, *this, line, translationId);
  }
}

TranslationTask::~TranslationTask()
{
}

void TranslationTask::Run()
{

  m_mgr->Decode();

  string out;

  out = m_mgr->OutputBest() + "\n";
  m_mgr->system.bestCollector->Write(m_mgr->GetTranslationId(), out);

  if (m_mgr->system.options.nbest.nbest_size) {
    out = m_mgr->OutputNBest();
    m_mgr->system.nbestCollector->Write(m_mgr->GetTranslationId(), out);
  }

  if (!m_mgr->system.options.output.detailed_transrep_filepath.empty()) {
    out = m_mgr->OutputTransOpt();
    m_mgr->system.detailedTranslationCollector->Write(m_mgr->GetTranslationId(), out);
  }

  delete m_mgr;
}

}

