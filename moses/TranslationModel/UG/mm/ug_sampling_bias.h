// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#pragma once

#include <map>
#include <vector>
#include <string>
#include <iostream>
// #include "moses/Util.h"
#include "ug_typedefs.h"
namespace sapt
{
  using tpt::id_type;

  std::string 
  query_bias_server(std::string const& url, 
                    std::string const& text, 
                    std::ostream* log);

  class SamplingBias
  {
  protected:
    std::vector<id_type> const* m_sid2docid;
  public:
    SamplingBias(std::vector<id_type> const* sid2docid);
    int loglevel;
    std::ostream* log;
    // Map to store the biasmap as you get it from the server:
    SPTR<std::map<std::string, float> > m_bias_map; 
    SPTR<std::map<std::string, float> const> getBiasMap();
    virtual float
    operator[](id_type const ID) const = 0;
    // returns (unnormalized bias) for the class of item ID

    virtual size_t size() const = 0;
    // number of classes

    virtual int
    GetClass(id_type const ID) const;
    // returns class/document/domain id of item ID
  };

  class
  DocumentBias : public SamplingBias
  {
    // std::vector<float> m_bias;
    std::map<id_type, float> m_bias;
  public:

    DocumentBias(std::vector<id_type> const& sid2doc,
                 std::map<std::string,id_type> const& docname2docid,
                 std::string const& server_url, 
                 std::string const& text,
                 std::ostream* log);

    DocumentBias(std::vector<id_type> const& sid2doc,
                 std::map<std::string,id_type> const& docname2docid,
                 std::map<std::string, float> const& context_weights,
                 std::ostream* log);

    void
    init_from_json
    ( std::string const& json,
      std::map<std::string,id_type> const& docname2docid,
      std::ostream* log );

    void
    init
    ( std::map<std::string,float> const& biasmap,
      std::map<std::string,id_type> const& docname2docid);

    float
    operator[](id_type const idx) const;

    size_t
    size() const;

    const std::map<id_type, float>& GetDocumentBiasMap() const;
  };

  class
  SentenceBias : public SamplingBias
  {
    std::vector<float> m_bias;
  public:
    SentenceBias(std::vector<float> const& bias,
                 std::vector<id_type> const* sid2docid = NULL);

    SentenceBias(size_t const s, float const f = 0, 
                 std::vector<id_type> const* sid2docid = NULL);

    float& operator[](id_type const idx);
    float  operator[](id_type const idx) const;
    size_t size() const;

  };

  class
  SamplingBiasAlways : public SamplingBias
  {
  public:
    SamplingBiasAlways(std::vector<id_type> const* sid2docid)
      : SamplingBias(sid2docid) {} 
      
    float operator[](id_type const idx)       { return 1; }
    float operator[](id_type const idx) const { return 1; }
    size_t size() const { return 0; }
  };

}

