#include "PhraseProperty.h"

namespace Moses
{

std::ostream& operator<<(std::ostream &out, const PhraseProperty &obj)
{
  obj.Print(out);
  return out;
}

void PhraseProperty::Print(std::ostream &out) const
{
  out << "Base phrase property";
}

}

