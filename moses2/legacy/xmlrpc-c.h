#pragma once

#ifdef HAVE_XMLRPC_C
#include <xmlrpc-c/base.hpp>
#else
namespace xmlrpc_c
{
class value;
}
#endif
