// -*- c++ -*-
#pragma once

#include <map>
#include<vector>
#include <string>
#include <iostream>
#include "moses/Util.h"
#include "ug_typedefs.h"
namespace Moses
{
  namespace bitext
  {
    using ugdiss::id_type;

    std::string query_bias_server(std::string const& url, std::string const& text);

    class SamplingBias 
    {
    public:
      int loglevel;
      std::ostream* log;
      virtual float 
      operator[](id_type const ID) const = 0;
      // returns (unnormalized bias) for the class of item ID

      virtual size_t size() const = 0; 
      // number of classes
      
      virtual id_type 
      GetClass(id_type const ID) const = 0;
      // returns class of item ID
    };
  
    class
    DocumentBias : public SamplingBias
    {
      std::vector<id_type> const& m_sid2docid;
      std::vector<float> m_bias;
		   
    public:
      
      DocumentBias(std::vector<id_type> const& sid2doc,
		   std::map<std::string,id_type> const& docname2docid,
		   std::string const& server_url, std::string const& text,
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
      
      id_type 
      GetClass(id_type const idx) const;

      float 
      operator[](id_type const idx) const;

      size_t 
      size() const;
    };

    class
    SentenceBias : public SamplingBias
    {
      std::vector<float> m_bias;
    public:
      SentenceBias(std::vector<float> const& bias);
      SentenceBias(size_t const s);

      id_type GetClass(id_type idx) const;

      float& operator[](id_type const idx); 
      float  operator[](id_type const idx) const; 
      size_t size() const;
      
    };

  }
}
