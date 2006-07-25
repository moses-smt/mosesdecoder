#include "Input.h"


InputType::InputType(long translationId) : m_translationId(translationId) {}
InputType::~InputType() {}

std::ostream& operator<<(std::ostream& out,InputType const& x) 
{
	x.Print(out); return out;
}
