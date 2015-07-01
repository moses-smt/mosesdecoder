#include "ug_sampling_bias.h"
#include <iostream>
#include <boost/foreach.hpp>
#include "moses/Timer.h"
// #include <curl/curl.h>
// #ifdef HAVE_CURLPP
// #include <curlpp/Options.hpp>
// #include <curlpp/cURLpp.hpp>
// #include <curlpp/Easy.hpp>
// #endif

// #ifdef WITH_MMT_BIAS_CLIENT
#include "ug_http_client.h"
// #endif

namespace Moses
{
  namespace bitext
  {
    using ugdiss::id_type;

    size_t ca_write_callback(void *ptr, size_t size, size_t nmemb, 
			     std::string* response) 
    {
      char const* c = reinterpret_cast<char const*>(ptr);
      *response += std::string(c, size * nmemb);
      return size * nmemb;
    }

    std::string 
    query_bias_server(std::string const& server, std::string const& context) 
    {
#if 0
      std::string query = server + uri_encode(context);
      std::string response;
      
      CURL* curl = curl_easy_init();
      UTIL_THROW_IF2(!curl, "Could not init curl.");
      curl_easy_setopt(curl, CURLOPT_URL, query.c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ca_write_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
      CURLcode res = curl_easy_perform(curl);
      curl_easy_cleanup(curl);
      return response;
#else
      std::string query = server+uri_encode(context);
      boost::asio::io_service io_service;
      Moses::http_client c(io_service, query);
      io_service.run();

      std::string response = c.content();
      std::cerr << "SERVER RESPONSE: " << response << std::endl;

      return c.content();
#endif
    }

//     // #ifdef WITH_MMT_BIAS_CLIENT
//     std::string
//     query_bias_server(std::string const& url, std::string const& text)
//     {
// #if 1
//       std::string query = url+uri_encode(text);
//       boost::asio::io_service io_service;
//       Moses::http_client c(io_service, query);
//       io_service.run();

//       std::string response = c.content();
//       std::cerr << "SERVER RESPONSE: " << response << std::endl;

//       return c.content();
// #else
//       return "";
// #endif
//     }
//     // #endif


    // std::string
    // query_bias_server(std::string const& url, int const port, 
    // 		      std::string const& context,
    // 		      std::string const& src_lang)
    // {
    //   char* response 
    // 	= ca_get_context(url.c_str(), port, context.c_str(), src_lang.c_str());
    //   UTIL_THROW_IF2(!response, "No response from server");
    //   std::string json = response;
    //   free(response);
    //   return json;
    // }

    DocumentBias 
    ::DocumentBias
    ( std::vector<id_type> const& sid2doc,
      std::map<std::string,id_type> const& docname2docid,
      std::string const& server_url, std::string const& text,
      std::ostream* log)
      : m_sid2docid(sid2doc)
      , m_bias(docname2docid.size(), 0)
    {
      // #ifdef HAVE_CURLPP
      Timer timer;
      if (log) timer.start(NULL);
      std::string json = query_bias_server(server_url, text);
      std::cerr << "SERVER RESPONSE " << json << std::endl;
      init_from_json(json, docname2docid, log);
      if (log) *log << "Bias query took " << timer << " seconds." << std::endl;
      // #endif
    }

    DocumentBias
    ::DocumentBias(std::vector<id_type> const& sid2doc,
                   std::map<std::string,id_type> const& docname2docid,
                   std::map<std::string, float> const& context_weights,
                   std::ostream* log)
                   : m_sid2docid(sid2doc)
                   , m_bias(docname2docid.size(), 0)
    {
    init(context_weights, docname2docid);
    }

    std::map<std::string, float>& SamplingBias::getBiasMap() {
      return m_bias_map;
    }

    void
    DocumentBias
    ::init_from_json
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
      if (log)
	{
	  BOOST_FOREACH(item& x, bias)
	    {
	      std::map<std::string,id_type>::const_iterator m;
	      m = docname2docid.find(x.first);
	      int docid = m != docname2docid.end() ? m->second : -1;
	      *log << "CONTEXT SERVER RESPONSE "
		   << "[" << docid << "] "
		   << x.first << " " << x.second << std::endl;
	    }
	}
      m_bias_map = bias;
      init(bias, docname2docid);

      // using xmlrpc_parse_json didn't always work (parser errors)
      // xmlrpc_value* b = xmlrpc_parse_json(env ,buf.str().c_str());
      // std::cerr << "|" << buf.str() << "|" << std::endl;
      // // if (b == NULL) std::cerr << "OOpS" << std::endl;
      // xmlrpc_c::value_struct v(b); //  = *b;
      // std::map<std::string, xmlrpc_c::value> const
      // 	bmap = static_cast<map<std::string, xmlrpc_c::value> >(v);
      // std::map<std::string, float> bias;
      // typedef std::map<std::string, xmlrpc_c::value>::value_type item;
      // float total = 0;
      // BOOST_FOREACH(item const& x, bmap)
      // 	{
      // 	  total += bias[x.first] = xmlrpc_c::value_double(x.second);
      // 	}
      // typedef std::map<std::string, float>::value_type fitem;
      // BOOST_FOREACH(fitem const& x, bias)
      // 	std::cerr << x.first << " " << x.second/total << std::endl;
      // // delete b;
    }

    void
    DocumentBias
    ::init(std::map<std::string,float> const& biasmap,
	   std::map<std::string,id_type> const& docname2docid)
    {
      typedef std::map<std::string, id_type>::value_type doc_record;
      float total = 0;
      BOOST_FOREACH(doc_record const& d, docname2docid)
	{
	  std::map<std::string, float>::const_iterator m = biasmap.find(d.first);
	  if (m != biasmap.end()) total += (m_bias[d.second] = m->second);
	  }
      if (total) { BOOST_FOREACH(float& f, m_bias) f /= total; }
      BOOST_FOREACH(doc_record const& d, docname2docid)
	std::cerr << "BIAS " << d.first << " " << m_bias[d.second] << std::endl;
    }

    id_type
    DocumentBias
    ::GetClass(id_type const idx) const
    {
      return m_sid2docid.at(idx);
    }

    float
    DocumentBias
    ::operator[](id_type const idx) const
    {
      UTIL_THROW_IF2(idx >= m_sid2docid.size(),
		     "Out of bounds: " << idx << "/" << m_sid2docid.size());
      return m_bias[m_sid2docid[idx]];
    }

    size_t
    DocumentBias
    ::size() const
    { return m_sid2docid.size(); }



    SentenceBias
    ::SentenceBias(std::vector<float> const& bias)
      : m_bias(bias) { }

    SentenceBias
    ::SentenceBias(size_t const s) : m_bias(s) { }

    id_type
    SentenceBias
    ::GetClass(id_type idx) const { return idx; }

    float&
    SentenceBias
    ::operator[](id_type const idx)
    {
      UTIL_THROW_IF2(idx >= m_bias.size(), "Out of bounds");
      return m_bias[idx];
    }

    float
    SentenceBias
    ::operator[](id_type const idx) const
    {
      UTIL_THROW_IF2(idx >= m_bias.size(), "Out of bounds");
      return m_bias[idx];
    }

    size_t
    SentenceBias
    ::size() const { return m_bias.size(); }

  }
}
