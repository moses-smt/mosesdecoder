// -*- c++ -*-
#pragma once

#include <string>
#include <map>
#include <vector>

#ifdef WITH_THREADS
#include <boost/thread.hpp>
#endif

#include "moses/Util.h"
#include "moses/ChartManager.h"
#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include "moses/StaticData.h"
#include "moses/ThreadPool.h"
#include "moses/TranslationModel/PhraseDictionaryMultiModel.h"
#include "moses/TreeInput.h"
#include "moses/TranslationTask.h"
#include <boost/shared_ptr.hpp>
#include <xmlrpc-c/base.hpp>

#include "Translator.h"

namespace MosesServer
{
class
TranslationRequest : public virtual Moses::TranslationTask
{
  boost::condition_variable& m_cond;
  boost::mutex& m_mutex;
  bool m_done;

  xmlrpc_c::paramList const& m_paramList;
  std::map<std::string, xmlrpc_c::value> m_retData;
  std::map<uint32_t,float> m_bias; // for biased sampling

  Translator* m_translator;
  std::string m_source_string, m_target_string;
  bool m_withAlignInfo;
  bool m_withWordAlignInfo;
  bool m_withGraphInfo;
  bool m_withTopts;
  bool m_withScoreBreakdown;
  uint64_t m_session_id; // 0 means none, 1 means new

  void
  parse_request();

  void
  parse_request(std::map<std::string, xmlrpc_c::value> const& req);

  virtual void
  run_chart_decoder();

  virtual void
  run_phrase_decoder();

  void
  pack_hypothesis(const Moses::Manager& manager, 
		  std::vector<Moses::Hypothesis const* > const& edges,
                  std::string const& key,
                  std::map<std::string, xmlrpc_c::value> & dest) const;

  void
  pack_hypothesis(const Moses::Manager& manager, Moses::Hypothesis const* h, 
		  std::string const& key,
                  std::map<std::string, xmlrpc_c::value> & dest) const;

  void
  add_phrase_aln_info(Moses::Hypothesis const& h,
                      std::vector<xmlrpc_c::value>& aInfo) const;

  void
  outputChartHypo(std::ostream& out, const Moses::ChartHypothesis* hypo);

  bool
  compareSearchGraphNode(const Moses::SearchGraphNode& a,
                         const Moses::SearchGraphNode& b);

  void
  insertGraphInfo(Moses::Manager& manager,
                  std::map<std::string, xmlrpc_c::value>& retData);
  void
  outputNBest(Moses::Manager const& manager,
              std::map<std::string, xmlrpc_c::value>& retData);

  void
  insertTranslationOptions(Moses::Manager& manager,
                           std::map<std::string, xmlrpc_c::value>& retData);
protected:
  TranslationRequest(xmlrpc_c::paramList const& paramList,
                     boost::condition_variable& cond,
                     boost::mutex& mut);

public:

  static
  boost::shared_ptr<TranslationRequest>
  create(Translator* translator,
	 xmlrpc_c::paramList const& paramList,
         boost::condition_variable& cond,
         boost::mutex& mut);


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
