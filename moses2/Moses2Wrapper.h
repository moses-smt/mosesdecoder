#pragma once
#include <string>
namespace Moses2 { 
	class Parameter;
	class System;
	class Moses2Wrapper
	{
		Parameter *m_param;
		System *m_system;

	public:
		Moses2Wrapper(const std::string &filePath);
		~Moses2Wrapper();
		std::string Translate(const std::string &input, long id);
		Moses2Wrapper* getInstance(const std::string& filePath);
		int getEngineVersion();

	};

}