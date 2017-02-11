#include "ContextParameters.h"
#include "../legacy/Parameter.h"

namespace Moses2
{

ContextParameters::
ContextParameters()
  : look_ahead(0), look_back(0)
{ }

bool
ContextParameters::
init(Parameter const& params)
{
  look_back = look_ahead = 0;
  params.SetParameter(context_string, "context-string", std::string(""));
  std::string context_window;
  params.SetParameter(context_window, "context-window", std::string(""));

  if (context_window == "")
    return true;

  if (context_window.substr(0,3) == "all") {
    look_back = look_ahead = std::numeric_limits<size_t>::max();
    return true;
  }

  size_t p = context_window.find_first_of("0123456789");
  if (p == 0)
    look_back = look_ahead = atoi(context_window.c_str());

  if (p == 1) {
    if (context_window[0] == '-')
      look_back  = atoi(context_window.substr(1).c_str());
    else if (context_window[0] == '+')
      look_ahead = atoi(context_window.substr(1).c_str());
    else
      UTIL_THROW2("Invalid specification of context window.");
  }

  if (p == 2) {
    if (context_window.substr(0,2) == "+-" ||
        context_window.substr(0,2) == "-+")
      look_back = look_ahead = atoi(context_window.substr(p).c_str());
    else
      UTIL_THROW2("Invalid specification of context window.");
  }
  return true;
}
}
