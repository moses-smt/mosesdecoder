/*
 * Server.cpp
 *
 *  Created on: 1 Apr 2016
 *      Author: hieu
 */
#include <iostream>
#include "../System.h"
#include "Server.h"
#include "Translator.h"
#include "../parameters/ServerOptions.h"

using namespace std;

namespace Moses2
{

Server::Server(ServerOptions &server_options, System &system)
  :m_server_options(server_options)
  ,m_translator(new Translator(*this, system))
{
  m_registry.addMethod("translate", m_translator);
}

Server::~Server()
{
  unlink(m_pidfile.c_str());
}

void Server::run(System &system)
{
  xmlrpc_c::serverAbyss myAbyssServer
  (xmlrpc_c::serverAbyss::constrOpt()
   .registryP(&m_registry)
   .portNumber(m_server_options.port) // TCP port on which to listen
   .logFileName(m_server_options.logfile)
   .allowOrigin("*")
   .maxConn(m_server_options.maxConn)
   .maxConnBacklog(m_server_options.maxConnBacklog)
   .keepaliveTimeout(m_server_options.keepaliveTimeout)
   .keepaliveMaxConn(m_server_options.keepaliveMaxConn)
   .timeout(m_server_options.timeout)
  );
  std::ostringstream pidfilename;
  pidfilename << "/tmp/moses-server." << m_server_options.port << ".pid";
  m_pidfile = pidfilename.str();
  std::ofstream pidfile(m_pidfile.c_str());

#ifdef _WIN32
  int thePid = GetCurrentProcessId();
#else
  int thePid = getpid();
#endif
  pidfile << thePid << std::endl;
  pidfile.close();
  cerr << "Listening on port " << m_server_options.port << std::endl;
  if (m_server_options.is_serial) {
    cerr << "Running server in serial mode." << std::endl;
    while(true) myAbyssServer.runOnce();
  } else myAbyssServer.run();

  std::cerr << "xmlrpc_c::serverAbyss.run() returned but it should not."
            << std::endl;
}

ServerOptions const&Server::options() const
{
  return m_server_options;
}


} /* namespace Moses2 */
