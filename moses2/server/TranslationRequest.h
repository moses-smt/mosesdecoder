// -*- c++ -*-
#pragma once

#include <string>
#include <map>
#include <vector>

#ifdef WITH_THREADS
#include <boost/thread.hpp>
#endif

#include <boost/shared_ptr.hpp>
#include <xmlrpc-c/base.hpp>
#include "../TranslationTask.h"

#include "Translator.h"

namespace Moses2
{
class Hypothesis;
class System;
class Manager;

class
  TranslationRequest : public virtual TranslationTask
{
protected:
  std::map<std::string, xmlrpc_c::value> m_retData;
  Translator* m_translator;

  boost::condition_variable& m_cond;
  boost::mutex& m_mutex;
  bool m_done;

  TranslationRequest(xmlrpc_c::paramList const& paramList,
                     boost::condition_variable& cond,
                     boost::mutex& mut,
                     System &system,
                     const std::string &line,
                     long translationId);

  void
  pack_hypothesis(const Manager& manager, Hypothesis const* h,
                  std::string const& key,
                  std::map<std::string, xmlrpc_c::value> & dest) const;

public:

  static
  boost::shared_ptr<TranslationRequest>
  create(Translator* translator,
         xmlrpc_c::paramList const& paramList,
         boost::condition_variable& cond,
         boost::mutex& mut,
         System &system,
         const std::string &line,
         long translationId);


  virtual bool
  DeleteAfterExecution() {
    return false;
  }

  bool
  IsDone() const {
    return m_done;
  }

  std::map<std::string, xmlrpc_c::value> const&
  GetRetData() {
    return m_retData;
  }

  void
  Run();


};

}
