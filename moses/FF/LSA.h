// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#pragma once

#include <string>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/unordered_map.hpp>
#include "moses/TranslationModel/UG/mm/tpt_tokenindex.h"

namespace Moses {

class LsaModel
{
  float const*       m_idf1;
  std::vector<float> m_S;
  float const*       m_term1;
  float const*       m_term2;
  uint64_t m_num_tokens1, m_num_tokens2;
  uint32_t m_num_dimensions;
  uint32_t m_num_cols;
  boost::iostreams::mapped_file_source m_idf1_file;
  boost::iostreams::mapped_file_source m_term1_file;
  boost::iostreams::mapped_file_source m_term2_file;
public:
  sapt::TokenIndex V1, V2;

  void 
  open(std::string const& bname, std::string const& L1, std::string const& L2);

  size_t 
  cols() const { return m_num_dimensions; }

  void 
  adapt(boost::unordered_map<uint32_t, uint32_t> const& wcnt, 
	float* dest) const;

  void 
  adapt(std::vector<std::string> const& document, float* dest) const;

  float const* tvec1(uint32_t const id) const
  { 
    return id < m_num_tokens1 ? m_term1 + (id * m_num_cols) : NULL; 
  }

  float const* tvec2(uint32_t const id) const 
  { 
    return id < m_num_tokens2 ? m_term2 + (id * m_num_cols) : NULL; 
  }
  
};

class LsaTermMatcher{

  LsaModel const* m_model;
  std::vector<float> m_document_vector;
public:
  LsaTermMatcher() : m_model(NULL) {};

  void init(LsaModel const* model, 
            boost::unordered_map<uint32_t, uint32_t> const& wordcounts);

  void init(LsaModel const* model, std::vector<std::string> const& doc);

  float operator()(uint32_t const id) const;
};


}
