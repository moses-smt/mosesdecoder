// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#pragma once

#include <vector>
#include <string>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#include <climits>
#include "moses/TranslationModel/UG/mm/tpt_tokenindex.h"
#include "ug_mm_dense_table.h"

namespace Moses {

class LsaModel
{
  typedef ug::mmTable::mmTable<float,2> dense_2d_table_t;
  typedef boost::unordered_map<std::string, uint32_t> str2row_map;
  str2row_map m_str2row1;
  str2row_map m_str2row2;
  dense_2d_table_t m_T, m_D;
  std::vector<float> m_S, m_sqrt_S, m_S_squared;
  std::vector<float> m_idf;
  std::string m_L1, m_L2;
  std::vector<std::string> m_rlabel;
  uint32_t m_last_L1_ID; 
  uint32_t m_last_L2_ID;
  float  m_log_total_docs;
  // float const*       m_idf1;
  // std::vector<float> m_S;
  // float const*       m_term1;
  // float const*       m_term2;
  // uint64_t m_num_tokens1, m_num_tokens2;
  // uint32_t m_num_dimensions;
  // uint32_t m_num_cols;
  // boost::iostreams::mapped_file_source m_idf1_file;
  // boost::iostreams::mapped_file_source m_term1_file;
  // boost::iostreams::mapped_file_source m_term2_file;

  void
  read_row_labels(std::string const& fname);

public:
  // sapt::TokenIndex V1, V2;

  uint32_t
  first_L1_ID() const
  {
    return m_last_L1_ID < m_last_L2_ID ? 0 : m_last_L2_ID + 1;
  }

  uint32_t
  last_L1_ID() const
  {
    return m_last_L1_ID;
  }

  uint32_t
  first_L2_ID() const
  {
    return m_last_L1_ID < m_last_L2_ID ? m_last_L1_ID + 1 : 0;
  }

  uint32_t
  last_L2_ID() const
  {
    return m_last_L2_ID;
  }

  std::string const&
  rlabel(uint32_t const row) const
  {
    return m_rlabel.at(row);
  }
  
  void 
  open(std::string const& bname,
       std::string const& L1,
       std::string const& L2,
       uint32_t const total_docs);

  uint32_t
  row1(std::string const& key) const
  {
    str2row_map::const_iterator m = m_str2row1.find(key);
    return m != m_str2row1.end() ? m->second : uint32_t(m_T.size());
  };

  uint32_t
  row2(std::string const& key) const
  {
    str2row_map::const_iterator m = m_str2row2.find(key);
    return m != m_str2row2.end() ? m->second : uint32_t(m_T.size());
  };

  uint32_t
  count_rows() const
  {
    return m_T.size(); 
  }
  
  uint32_t
  count_cols() const
  {
    return m_T.size(1); 
  }

  float
  term_similarity(uint32_t const row1, uint32_t const row2)
  {
    if (row1 >= m_T.size() || row2 >= m_T.size()) return 0;
    float ret=0;
    float a=0, b=0;
    float const* x = &m_T[row1][0];
    float const* y = &m_T[row2][0];
    for (size_t i = 0; i < m_S.size(); ++i)
      {
        float s = m_S[i] * m_S[i];
        ret += s * x[i] * y[i];
        a   += s * x[i] * x[i];
        b   += s * y[i] * y[i];
      }
    return ret ? ret / (sqrt(a) * sqrt(b)) : 0;
  }

  float
  term_doc_sim(uint32_t const r, std::vector<float> const& doc) const
  {
    if (r >= m_T.size()) return 0;
    float const* x = &m_T[r][0];
    float a=0, b=0, c = 0;
    for (size_t i = 0; i < m_S.size(); ++i)
      {
        a += m_S[i] * x[i] * doc[i];
        b += m_S[i] * x[i] * x[i];
        c += m_S[i] * doc[i] * doc[i];
      }
    return a ? a / (sqrt(b) * sqrt(c)) : 0;
  }

  float
  term_doc_sim(std::vector<float> const& term, std::vector<float> const& doc) const
  {
    float const* x = &term[0];
    float a=0, b=0, c = 0;
    for (size_t i = 0; i < m_S.size(); ++i)
      {
        a += m_S[i] * x[i] * doc[i];
        b += m_S[i] * x[i] * x[i];
        c += m_S[i] * doc[i] * doc[i];
      }
    return a ? a / (sqrt(b) * sqrt(c)) : 0;
  }

  void
  fold_in(uint32_t const row_id,
          uint32_t const count,
          std::vector<float>& dest) const
  {
    if (row_id >= m_T.size()) return;
    if (dest.size() != m_T.size(1)) throw "Dimensions do not match!";
    ug::mmTable::SubTable<float,1> row = m_T[row_id];
    float w = (1 + log(count)) * m_idf[row_id];

    for (size_t i = 0; i < m_T.size(1); ++i)
      dest[i] += w * row[i];
  }

  void
  fold_in_raw_counts(uint32_t const row_id, uint32_t const count,
                     std::vector<float>& dest)
  {
    if (row_id >= m_T.size()) return;
    if (dest.size() != m_T.size(1)) throw "Dimensions do not match!";
    ug::mmTable::SubTable<float,1> row = m_T[row_id];
    for (size_t i = 0; i < m_T.size(1); ++i)
      dest[i] += count * row[i];
  }

  // void
  // fold_in(std::vector<uint32_t> const& row_ids, std::vector<float>& vec)
  // {
  //   vec.assign(m_T.size(1), 0);
  //   BOOST_FOREACH(uint32_t r, row_ids)
  //     fold_in(r,vec);
  // }

  std::vector<float> const&
  S() const { return m_S; }

  dense_2d_table_t const&
  T() const { return m_T; }
  
  std::vector<float> const&
  sqrt_S() const { return m_sqrt_S; }

  std::vector<float> const&
  S_squared() const { return m_S_squared; }

  
  // size_t 
  // cols() const { return m_num_dimensions; }

  // void 
  // adapt(boost::unordered_map<uint32_t, uint32_t> const& wcnt, 
	// float* dest) const;

  // void 
  // adapt(std::vector<std::string> const& document, float* dest) const;

  // float const* tvec1(uint32_t const id) const
  // { 
  //   return id < m_num_tokens1 ? m_term1 + (id * m_num_cols) : NULL; 
  // }

  // float const* tvec2(uint32_t const id) const 
  // { 
  //   return id < m_num_tokens2 ? m_term2 + (id * m_num_cols) : NULL; 
  // }
  
};

class LsaTermMatcher
{
  
  LsaModel const* m_model;
  std::vector<float> m_document_vector;
  
public:
  float
  operator()(std::string const& word) const;

  float
  operator()(std::string const& word, std::vector<float> const& v) const;

  std::vector<float> const& 
  fold_in(LsaModel const* model, std::vector<std::string> const& doc);
  
  std::vector<float> const& 
  fold_in(LsaModel const* model, std::string const& line);
  
  std::vector<float> const& 
  fold_in_word(LsaModel const* model, std::string const& w);

  std::vector<float> const&
  embedding() const
  {
    return m_document_vector;
  }
  
  // public:
//   LsaTermMatcher() : m_model(NULL) {};

//   void init(LsaModel const* model, 
//             boost::unordered_map<uint32_t, uint32_t> const& wordcounts);

//   void init(LsaModel const* model, std::vector<std::string> const& doc);

//   float operator()(uint32_t const id) const;
};


}
