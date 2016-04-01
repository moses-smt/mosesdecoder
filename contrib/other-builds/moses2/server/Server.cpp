/*
 * Server.cpp
 *
 *  Created on: 1 Apr 2016
 *      Author: hieu
 */
#include <iostream>
#include "Server.h"
#include "../System.h"

using namespace std;

namespace Moses2
{

Server::Server()
{

}

Server::~Server()
{
  // TODO Auto-generated destructor stub
}

void Server::run(System &system)
{
  xmlrpc_c::serverAbyss myAbyssServer
    (xmlrpc_c::serverAbyss::constrOpt()
     .registryP(&m_registry)
     .portNumber(system.port) // TCP port on which to listen
     .logFileName(system.logfile)
     .allowOrigin("*")
     .maxConn(system.maxConn)
     .maxConnBacklog(system.maxConnBacklog)
     .keepaliveTimeout(system.keepaliveTimeout)
     .keepaliveMaxConn(system.keepaliveMaxConn)
     .timeout(system.timeout)
     );
  std::ostringstream pidfilename;
  pidfilename << "/tmp/moses-server." << system.port << ".pid";
  m_pidfile = pidfilename.str();
  std::ofstream pidfile(m_pidfile.c_str());
  pidfile << getpid() << std::endl;
  pidfile.close();
  cerr << "Listening on port " << system.port << std::endl;
  if (system.is_serial)
    {
      cerr << "Running server in serial mode." << std::endl;
      while(true) myAbyssServer.runOnce();
    }
  else myAbyssServer.run();

  std::cerr << "xmlrpc_c::serverAbyss.run() returned but it should not."
            << std::endl;
}

} /* namespace Moses2 */
