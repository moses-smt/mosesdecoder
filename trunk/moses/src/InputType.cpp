#include "InputType.h"

InputType::InputType(long translationId) : m_translationId(translationId) {}
InputType::~InputType() {}

TO_STRING_BODY(InputType);

std::ostream& operator<<(std::ostream& out,InputType const& x) 
{
	x.Print(out); return out;
}
