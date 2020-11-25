#include "Moses2Wrapper.h"
#include "System.h"
#include "legacy/Parameter.h"
#include "TranslationTask.h"
using namespace std;
namespace Moses2 {
	//summary ::  need to update the LM path at runtime with complete artifact path.
	void Moses2Wrapper::UpdateLMPath(const std::string& filePath) {

		char sep = '/';

		#ifdef _WIN32
				sep = '\\';
		#endif
		auto file = filePath.substr(filePath.find_last_of(sep) + 1);
		auto path = filePath.substr(0, filePath.find_last_of(sep));
		auto a = m_param->GetParam("feature");
		std::vector<std::string> feature;
		for (int i = 0; i < a->size(); i++) {
			auto abc = Tokenize(a->at(i));
			if (*abc.begin() == "KENLM") {
				string s = "";
				for (int k = 0; k < abc.size(); k++) {
					if (abc.at(k).find("path=") != string::npos) {
						auto lm = abc.at(k).substr(abc.at(k).find_last_of("=") + 1);
						s = s + "path=" + path + sep + lm + " ";
					}
					else {
						s = s + abc.at(k) + " ";
					}
				}
				feature.push_back(s.erase(s.find_last_not_of(" \n\r\t") + 1));
			}
			else {
				feature.push_back(a->at(i));
			}
		}
		m_param->OverwriteParam("feature", feature);
	}

	Moses2Wrapper::Moses2Wrapper(const std::string &filePath) {
		m_param = new Parameter();
		m_param->LoadParam(filePath);
		UpdateLMPath(filePath);
		m_system = new System(*m_param);
	}
	std::string Moses2Wrapper::Translate(const std::string &input , long id) {
		TranslationTask task(*m_system, input, id);
		return task.ReturnTranslation();
	}
	Moses2Wrapper::~Moses2Wrapper() {
		delete m_param;
		delete  m_system;
	}
}