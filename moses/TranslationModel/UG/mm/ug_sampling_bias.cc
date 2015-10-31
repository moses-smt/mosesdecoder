// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#include "ug_sampling_bias.h"
#include <iostream>
#include <boost/foreach.hpp>
#include "moses/Util.h"
#ifndef NO_MOSES
#include "moses/Timer.h"
#endif

// #ifdef WITH_MMT_BIAS_CLIENT
#include "ug_http_client.h"
// #endif

namespace sapt
{
  using tpt::id_type;

  std::string 
  query_bias_server(std::string const& server, 
                    std::string const& context, 
                    std::ostream* log) 
  {
    std::string query = server + Moses::uri_encode(context);
    boost::asio::io_service io_service;
    Moses::http_client c(io_service, query, log);
    io_service.run();
    
    if (log)
      {
        std::string response = c.content();
        *log << "SERVER RESPONSE: " << response << std::endl;
      }
    if (c.content().size() == 0)
      {
        if (log) *log << "BIAS SERVER ERROR: " << c.error_msg() << std::endl;
      }
    return c.content();
  }
    
  SamplingBias::
  SamplingBias(std::vector<id_type> const* sid2doc)
    : m_sid2docid(sid2doc)
  { }

  int 
  SamplingBias::
  GetClass(id_type const idx) const
  {
    return m_sid2docid ? m_sid2docid->at(idx) : -1;
  }

  DocumentBias::
  DocumentBias(std::vector<id_type> const& sid2doc,
               std::map<std::string,id_type> const& docname2docid,
               std::string const& server_url, 
               std::string const& text,
               std::ostream* _log)
    : SamplingBias(&sid2doc)
  {
    this->log = _log;
#ifndef NO_MOSES
    Moses::Timer timer;
    if (_log) timer.start(NULL);
#endif
    std::string json = query_bias_server(server_url, text, _log);
      
    init_from_json(json, docname2docid, log);
#ifndef NO_MOSES
    if (_log) *_log << "Bias query took " << timer << " seconds." << std::endl;
#endif
  }

  DocumentBias::
  DocumentBias(std::vector<id_type> const& sid2doc,
               std::map<std::string,id_type> const& docname2docid,
               std::map<std::string, float> const& context_weights,
               std::ostream* _log)
    : SamplingBias(&sid2doc)
  {
    this->log = _log;
    init(context_weights, docname2docid);
  }

  SPTR<std::map<std::string, float> const> 
  SamplingBias::
  getBiasMap() {
    return m_bias_map;
  }

  const std::map<id_type, float>&
  DocumentBias::
  GetDocumentBiasMap() const {
    return m_bias;
  }

  void
  DocumentBias::
  init_from_json
  ( std::string const& json, std::map<std::string,id_type> const& docname2docid,
    std::ostream* log)
  { // poor man's special purpose json parser for responses from the
    // MMT bias server

    std::string d; float total = 0; std::map<std::string,float> bias;
    size_t i = 0; while (i < json.size() && json[i] != '"') ++i;
    while (++i < json.size())
      {
        size_t  k = i; while (i < json.size() && json[i] != '"') ++i;
        if (i >= json.size())  break;
        float& f = bias[json.substr(k,i-k)];
        while (++i < json.size() && json[i] != ':');
        k = ++i;
        while (++i < json.size() && json[i] != ',' && json[i] != '}');
        total += (f = atof(json.substr(k, i-k).c_str()));
        k = ++i; while (i < json.size() && json[i] != '"') ++i;
      }

    typedef std::pair<std::string const,float> item;
    if (total) { BOOST_FOREACH(item& x, bias) { x.second /= total; } }
    init(bias, docname2docid);

  }

  void
  DocumentBias::
  init(std::map<std::string,float> const& biasmap,
       std::map<std::string,id_type> const& docname2docid)
  {
    typedef std::map<std::string, float>::value_type bias_record;
    float total = 0;
    m_bias_map.reset(new std::map<std::string,float>(biasmap));
    BOOST_FOREACH(bias_record const& b, biasmap)
      {
        std::map<std::string, id_type>::const_iterator m;
        m = docname2docid.find(b.first);
        if (m != docname2docid.end()) 
          total += (m_bias[m->second] = b.second);
      }
    if (total) 
      { 
        typedef std::map<id_type, float>::value_type item;
        BOOST_FOREACH(item& i, m_bias) i.second /= total; 
      }
      
    if (log)
      {
        BOOST_FOREACH(bias_record const& b, biasmap)
          {
            std::map<std::string, id_type>::const_iterator m;
            m = docname2docid.find(b.first);
            if (m != docname2docid.end()) 
              *log << "BIAS " << b.first << " " << m_bias[m->second] 
                   << std::endl;
            else
              *log << "WARNING: bias reported for unknown document " 
                   << b.first << std::endl;
          }
      }
  }

  float
  DocumentBias::
  operator[](id_type const idx) const
  {
    std::map<id_type, float>::const_iterator m;
    m = m_bias.find((*m_sid2docid)[idx]);
    return m != m_bias.end() ? m->second : 0;
  }

  size_t
  DocumentBias::
  size() const
  { 
    return m_sid2docid->size(); 
  }



  SentenceBias::
  SentenceBias(std::vector<float> const& bias,
               std::vector<id_type> const* sid2doc)
    : SamplingBias(sid2doc)
    , m_bias(bias) 
  { }

  SentenceBias::
  SentenceBias(size_t const s, float const f,
               std::vector<id_type> const* sid2doc)
      
    : SamplingBias(sid2doc)
    , m_bias(s,f) 
  { }

  float&
  SentenceBias::
  operator[](id_type const idx)
  {
    UTIL_THROW_IF2(idx >= m_bias.size(), "Out of bounds");
    return m_bias[idx];
  }

  float
  SentenceBias::
  operator[](id_type const idx) const
  {
    UTIL_THROW_IF2(idx >= m_bias.size(), "Out of bounds");
    return m_bias[idx];
  }

  size_t
  SentenceBias::
  size() const { return m_bias.size(); }

}

