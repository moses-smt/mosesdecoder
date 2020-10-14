#include "Moses2Wrapper.h"
#include "System.h"
#include "legacy/Parameter.h"
#include "TranslationTask.h"
using namespace std;
namespace Moses2 {
	Moses2Wrapper::Moses2Wrapper(const std::string &filePath) {
		m_param = new Parameter();
		m_param->LoadParam(filePath);
		m_system = new System(*m_param);
	}
	std::string Moses2Wrapper::Translate(const std::string &input) {
		//create id
		long a = 11234567;
		TranslationTask task(*m_system, input, a);
		std::string  translation = task.RunTranslation();
		//delete translation;
		return translation;
	}
}