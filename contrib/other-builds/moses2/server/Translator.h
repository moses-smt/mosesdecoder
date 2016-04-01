/*
 * Translator.h
 *
 *  Created on: 1 Apr 2016
 *      Author: hieu
 */

#pragma once
#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>

namespace Moses2
{

class Translator : public xmlrpc_c::method
{
public:
  Translator();
  virtual ~Translator();

  void execute(xmlrpc_c::paramList const& paramList,
   xmlrpc_c::value *   const  retvalP);

};

} /* namespace Moses2 */

