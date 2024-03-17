#pragma once
#include <string>

namespace Moses2 {
	class Parameter;
	class System;
	extern "C" {
		enum MosesApiErrorCode {
			MS_API_OK,
			MS_API_E_FAILURE,
			MS_API_E_INPUT,
			MS_API_E_TIMEOUT
		};
	}
	class Moses2Wrapper
	{
		Parameter* m_param;
		System* m_system;

	public:
		Moses2Wrapper(const std::string& filePath);
		~Moses2Wrapper();
		std::string Translate(const std::string& input, long id, bool nbest);
		void UpdateLMPath(const std::string& filePath);

		static char* CopyString(const char* str);
		static void Free(void* ptr);
	};

}