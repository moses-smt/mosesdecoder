// -*- c++ -*-
#pragma once

#include "moses/Util.h"
#include "moses/ChartManager.h"
#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include "moses/StaticData.h"
#include "moses/ThreadPool.h"

#if PT_UG
#include "moses/TranslationModel/UG/mmsapt.h"
#endif

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>


namespace MoseServer
{
  class 
  Updater: public xmlrpc_c::method
  {
    std::string m_src, m_trg, m_aln;
    bool m_bounded, m_add2ORLM;

  public:
    Updater();
    
    void
    execute(xmlrpc_c::paramList const& paramList,
	    xmlrpc_c::value * const  retvalP);

    void 
    breakOutParams(const params_t& params);
      
  };

}
