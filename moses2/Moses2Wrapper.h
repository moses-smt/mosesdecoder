#pragma once
#include <string>
#include <string.h>
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
		std::string Translate(const std::string& input, long id);
		void UpdateLMPath(const std::string& filePath);
		int getEngineVersion();

		static char* CopyString(const char* str) {
			int32_t size = (int32_t)strlen(str);
			char* obj = (char*)malloc(size + 1);
			memcpy(obj, str, size);
			obj[size] = '\0';
			return obj;
		}
		static void Free(void* ptr) {
			free(ptr);
		}
	};

}