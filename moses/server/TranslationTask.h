// -*- c++ -*-
#pragma once

#include <string>
#include <map>
#include <vector>

#include <xmlrpc-c/base.hpp>


#ifdef WITH_THREADS
#include <boost/thread.hpp>
#endif

#include "moses/Util.h"
#include "moses/ChartManager.h"
#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include "moses/StaticData.h"
#include "moses/ThreadPool.h"

namespace MosesServer
{
  class 
  TranslationTask : public virtual Moses::Task
  {
    boost::condition_variable& m_cond;
    boost::mutex& m_mutex;

    xmlrpc_c::paramList const& m_paramList;
    std::map<std::string, xmlrpc_c::value> m_retData;
    bool m_done;
    std::map<uint32_t,float> m_bias; // for biased sampling
    
    std::string m_source, m_target;
    bool m_withAlignInfo;
    bool m_withWordAlignInfo;
    bool m_withGrapInfo;
    bool m_withTopts;
    bool m_reportAllFactors;
    bool m_nbestDistinct;
    bool m_withScoreBreakdown;
    size_t m_nbestSize;

    void 
    parse_request();
    
    virtual void
    run_chart_decoder();

    virtual void
    run_phrase_decoder();
    
    void 
    pack_hypothesis(vector<Hypothesis const* > const& edges, 
		    std::string const& key, 
		    std::map<std::string, xmlrpc_c::value> & dest) const;
    
    void 
    output_phrase(ostream& out, Phrase const& phrase);

    void 
    add_phrase_aln(Hypothesis const& h, std::vector<xmlrpc_c::value>& aInfo);

    void 
    outputChartHypo(ostream& out, const ChartHypothesis* hypo);

    bool 
    compareSearchGraphNode(const SearchGraphNode& a, const SearchGraphNode b);

    void 
    insertGraphInfo(Manager& manager, 
		    std::map<std::string, xmlrpc_c::value>& retData); 
    void 
    outputNBest(Manager const& manager, 
		std::map<std::string, xmlrpc_c::value>& retData);

    void 
    insertTranslationOptions(Manager& manager, 
			     std::map<std::string, xmlrpc_c::value>& retData);
    
  public:
    
    TranslationTask(xmlrpc_c::paramList const& paramList, 
		    boost::condition_variable& cond, 
		    boost::mutex& mut);
    
    virtual bool 
    DeleteAfterExecution() { return false; }
    
    bool 
    IsDone() const { return m_done; }
    
    std::map<std::string, xmlrpc_c::value> const& 
    GetRetData() { return m_retData; }
    
    void 
    Run();
    
    
  };

}
