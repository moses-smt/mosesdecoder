#include "Moses2Wrapper.h"
#include <iostream>
using namespace std;
using namespace Moses2;

extern "C" __declspec(dllexport) Moses2::Moses2Wrapper * __stdcall CreateMosesSystem(const char* filePath) {
	return new Moses2::Moses2Wrapper(filePath);
}

extern "C" __declspec(dllexport) int __stdcall GetMosesSystem(const char* filePath, Moses2::Moses2Wrapper ** pObject) {
		*pObject = new Moses2::Moses2Wrapper(filePath);
		return 1;
}

extern "C" __declspec(dllexport) int __stdcall MosesTranslate(Moses2::Moses2Wrapper * pObject, long id, const char* input, char* output, int strlen) {
	if (pObject != NULL)
	{
		std::string tr = pObject->Translate(input, id);
		std::copy(tr.begin(), tr.end(), output);
		output[std::min(strlen - 1, (int)tr.size())] = 0;
		return 1;
	}
	else {
		return 0;
	}
}
extern "C" __declspec(dllexport) int __stdcall ReleaseSystem(Moses2::Moses2Wrapper ** pObject) {
	if (*pObject != NULL)
	{
		delete *pObject;
		*pObject = NULL;
		return 1;
	}
	else {
		return 0;
	}
}
extern "C" __declspec(dllexport) string __stdcall GetEngineVersion() {
	return "1.0";
}