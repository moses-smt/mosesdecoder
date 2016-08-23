// -*- c++ -*-

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>

namespace MosesServer
{
class
  Optimizer : public xmlrpc_c::method
{
public:
  Optimizer();
  void execute(xmlrpc_c::paramList const& paramList,
               xmlrpc_c::value *   const  retvalP);
};
}
