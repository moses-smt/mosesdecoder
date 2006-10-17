// $Id: InputType.cpp 666 2006-08-11 21:04:38Z eherbst $

#include "InputType.h"

InputType::InputType(long translationId) : m_translationId(translationId) {}
InputType::~InputType() {}

TO_STRING_BODY(InputType);

std::ostream& operator<<(std::ostream& out,InputType const& x) 
{
	x.Print(out); return out;
}
