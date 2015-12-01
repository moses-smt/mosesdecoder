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

	const Hypothesis *bestHypo = m_mgr->GetBestHypothesis();
	if (bestHypo) {
		bestHypo->OutputToStream(cout);
		cerr << "BEST TRANSLATION: " << *bestHypo;
	}
	else {
		cerr << "NO TRANSLATION";
	}
	cout << endl;
	cerr << endl;

}
