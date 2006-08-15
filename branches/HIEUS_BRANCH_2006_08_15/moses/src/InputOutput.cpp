// $Id$
#include "InputOutput.h"
#include "InputType.h"

InputOutput::InputOutput() : sentenceId(0) {}

InputOutput::~InputOutput() {}

void InputOutput::Release(InputType *s) {delete s;}


InputType* InputOutput::GetInput(InputType *inputType
																 , std::istream &inputStream
																 , const std::vector<FactorType> &factorOrder
																 , FactorCollection &factorCollection) 
{
	if(inputType->Read(inputStream,factorOrder,factorCollection)) 
		{
			inputType->SetTranslationId(sentenceId++);
			return inputType;
		}
	else 
		{
			delete inputType;
			return 0;
		}
}

