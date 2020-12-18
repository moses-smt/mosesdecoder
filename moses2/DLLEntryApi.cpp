#include "Moses2Wrapper.h"
#include <iostream>
#ifdef WIN32
	#include <windows.h>
#endif // DEBUG

#if defined(_MSC_VER)
//  Microsoft 
	#define EXPORT __declspec(dllexport)
	#define IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
//  GCC
	#define HRESULT int
	#define EXPORT __attribute__((visibility("default")))
	#define __stdcall
	#define IMPORT
	#define S_OK 0
	#define E_FAIL 1
#else
//  do nothing and hope for the best?
	#define EXPORT
	#define IMPORT
	#pragma warning Unknown dynamic link import/export semantics.
#endif

using namespace std;
using namespace Moses2;

extern "C" EXPORT HRESULT __stdcall GetMosesSystem(const char* filePath, Moses2::Moses2Wrapper ** pObject) {
	if (*pObject == NULL) {
		*pObject = new Moses2::Moses2Wrapper(filePath);
		return S_OK;
	}
	else {
		return E_FAIL;
	}
}

extern "C" EXPORT HRESULT __stdcall MosesTranslate(Moses2::Moses2Wrapper * pObject, long id, const char* input, char* output, int strlen) {
	if (pObject != NULL)
	{
		std::string tr = pObject->Translate(input, id);
		std::copy(tr.begin(), tr.end(), output);
		output[std::min(strlen - 1, (int)tr.size())] = 0;
		return S_OK;
	}
	else {
		return E_FAIL;
	}
}
extern "C" EXPORT int __stdcall ReleaseSystem(Moses2::Moses2Wrapper ** pObject) {
	if (*pObject != NULL)
	{
		delete *pObject;
		*pObject = NULL;
		return S_OK;
	}
	else {
		return E_FAIL;
	}
}
extern "C" EXPORT string __stdcall GetEngineVersion() {
	return "1.0";
}