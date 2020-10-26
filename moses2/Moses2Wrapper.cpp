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
	std::string Moses2Wrapper::Translate(const std::string &input , long id) {
		TranslationTask task(*m_system, input, id);
		std::string  translation = task.ReturnTranslation();
		return translation;
	}
	Moses2Wrapper* Moses2Wrapper::getInstance(const std::string& filePath) {
		Moses2Wrapper *instance = new Moses2Wrapper(filePath);
		return instance;
	}
	Moses2Wrapper::~Moses2Wrapper() {
		cout << "Destructor is called ";
	}	
}