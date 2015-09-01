// -*- mode: c++; cc-style: gnu -*-
#include "ServerOptions.h"
#include <boost/foreach.hpp>
#include <string>
namespace Moses
{

// parse the session timeout specifciation for moses server
// Format is "<number>d[<number>[h[<number>m[<number>s]]]]".
// If none of 'dhms' is given, it is assumed that it's seconds.
// Specs can be combined, e.g. 2h30m, although it's probably nonsense
// to be so specific.
size_t 
parse_timespec(std::string const& spec)
{
  size_t t = 0, timeout = 0;
  BOOST_FOREACH(char const& c, spec)
    {
      if (c >= '0' && c <= '9') 
	{
	  t = t * 10 + c - '0';
	}
      else 
	{
	  if (c == 'd')      timeout  = t * 24 * 3600;
	  else if (c == 'h') timeout += t * 3600;
	  else if (c == 'm') timeout += t * 60;
	  else if (c == 's') timeout += t;
	  else UTIL_THROW2("Can't parse specification '" << spec 
			   << " at " << HERE);
	  t = 0;
	}
    }
  return timeout;
}

ServerOptions::
ServerOptions(Parameter const& P)
{ 
  init(P);
}

bool
ServerOptions::
init(Parameter const& P)
{
  P.SetParameter(this->port, "server-port", 8080);
  P.SetParameter(this->is_serial, "serial", false);
  P.SetParameter(this->logfile, "server-log", std::string("/dev/null"));
  P.SetParameter(this->num_threads, "threads", uint32_t(10));
  P.SetParameter(this->session_cache_size, "session-cache_size",25UL);
  std::string timeout_spec;
  P.SetParameter(timeout_spec, "session-timeout",std::string("30m"));
  this->session_timeout = parse_timespec(timeout_spec);
  return true;
}
} // namespace Moses
