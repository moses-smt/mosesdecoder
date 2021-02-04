#include "Moses2Wrapper.h"
#include <iostream>
#include <string.h>


// Generic helper definitions for shared library support
#if defined _WIN32
#define IMPORT __declspec(dllimport)
#define EXPORT __declspec(dllexport)
#else    // !(defined _WIN32 || defined __CYGWIN__) -- i.e., not Windows
#define __stdcall
#if __GNUC__ >= 4
#define IMPORT __attribute__ ((visibility ("default")))
#define EXPORT __attribute__ ((visibility ("default")))
#else   // __GNUC__ < 4, which does not support the __attribute__ tag
#define IMPORT
#define EXPORT
#endif  // __GNUC__ >= 4
#endif 


using namespace std;
using namespace Moses2;

extern "C" EXPORT MosesApiErrorCode __stdcall GetMosesSystem(const char* filePath, Moses2::Moses2Wrapper * *pObject) {

	if (*pObject == NULL) {
		*pObject = new Moses2::Moses2Wrapper(filePath);
		return MS_API_OK;
	}
	else {
		return MS_API_E_FAILURE;
	}
}

extern "C" EXPORT MosesApiErrorCode __stdcall Translate(Moses2::Moses2Wrapper * pObject, long id, const char* input, char** output) {
	if (pObject != NULL)
	{
		std::string tr = pObject->Translate(input, id);
		*output = Moses2Wrapper::CopyString(tr.c_str());
		return MS_API_OK;
	}
	else {
		return MS_API_E_FAILURE;
	}
}

extern "C" EXPORT MosesApiErrorCode __stdcall FreeMemory(char* output) {
	if (output != nullptr) {
		Moses2Wrapper::Free(output);
		return MS_API_OK;
	}
	else {
		return MS_API_E_FAILURE;
	}
}

extern "C" EXPORT MosesApiErrorCode __stdcall ReleaseSystem(Moses2::Moses2Wrapper **pObject) {
	if (*pObject != NULL)
	{
		delete* pObject;
		*pObject = NULL;
		return MS_API_OK;
	}
	else {
		return MS_API_E_FAILURE;
	}
}

extern "C" EXPORT MosesApiErrorCode __stdcall EngineVersion() {
	//std::cout << "windows build on v1142/ msvc 14.27.29110"<< std::endl;
	std::cout << "0.0.1" << std::endl;
	return MS_API_OK;
}