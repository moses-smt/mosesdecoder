// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include <boost/foreach.hpp>
#include <string>
#include "../legacy/Parameter.h"
#include "ServerOptions.h"
#include "../legacy/Util2.h"
#include "util/exception.hh"

namespace Moses2
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
  BOOST_FOREACH(char const& c, spec) {
    if (c >= '0' && c <= '9') {
      t = t * 10 + c - '0';
    } else {
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
ServerOptions()
  : is_serial(false)
  , numThreads(15) // why 15?
  , sessionTimeout(1800) // = 30 min
  , sessionCacheSize(25)
  , port(8080)
  , maxConn(15)
  , maxConnBacklog(15)
  , keepaliveTimeout(15)
  , keepaliveMaxConn(30)
  , timeout(15)
{ }

ServerOptions::
ServerOptions(Parameter const& P)
{
  init(P);
}

bool
ServerOptions::
init(Parameter const& P)
{
  // Settings for the abyss server
  P.SetParameter(this->port, "server-port", 8080);
  P.SetParameter(this->is_serial, "serial", false);
  P.SetParameter(this->logfile, "server-log", std::string("/dev/null"));
  P.SetParameter(this->numThreads, "threads", uint32_t(15));

  // defaults reflect recommended defaults (according to Hieu)
  // -> http://xmlrpc-c.sourceforge.net/doc/libxmlrpc_server_abyss.html#max_conn
  P.SetParameter(this->maxConn,"server-maxconn", 15);
  P.SetParameter(this->maxConnBacklog,"server-maxconn-backlog", 15);
  P.SetParameter(this->keepaliveTimeout,"server-keepalive-timeout", 15);
  P.SetParameter(this->keepaliveMaxConn,"server-keepalive-maxconn", 30);
  P.SetParameter(this->timeout,"server-timeout",15);

  // the stuff below is related to Moses translation sessions
  std::string timeout_spec;
  P.SetParameter(timeout_spec, "session-timeout",std::string("30m"));
  this->sessionTimeout = parse_timespec(timeout_spec);
  P.SetParameter(this->sessionCacheSize, "session-cache_size", size_t(25));

  return true;
}
} // namespace Moses
