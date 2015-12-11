#include "BookkeepingOptions.h"

namespace Moses
{

bool
BookkeepingOptions::
init(Parameter const& P)
{
  bool& x = need_alignment_info;
  P.SetParameter(x, "print-alignment-info", false);
  if (!x) P.SetParameter(x, "print-alignment-info-in-n-best", false);
  if (!x) {
    PARAM_VEC const* params = P.GetParam("alignment-output-file");
    x = params && params->size();
  }
  return true;
}

BookkeepingOptions::
BookkeepingOptions()
  : need_alignment_info(false)
{ }

}
