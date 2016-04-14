// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#include "LSA.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/foreach.hpp>

namespace Moses {

void
LsaModel::
open(std::string const& bname, std::string const& L1, std::string const& L2)
{
  V1.open(bname + L1 + ".tdx");
  V2.open(bname + L2 + ".tdx");

  m_idf1_file.open(bname + L1 + ".idf");
  m_term1_file.open(bname + L1 + ".lsi");
  m_term2_file.open(bname + L2 + ".lsi");
  m_num_tokens1 = *reinterpret_cast<uint64_t const*>(m_term1_file.data());
  m_num_tokens2 = *reinterpret_cast<uint64_t const*>(m_term2_file.data());
  m_num_cols = *reinterpret_cast<uint32_t const*>(m_term1_file.data() + 8);
  m_num_dimensions = m_num_cols - 1;

  m_term1 = reinterpret_cast<float const*>(m_term1_file.data() + 12);
  m_term2 = reinterpret_cast<float const*>(m_term2_file.data() + 12);

  for (size_t i = 0; i < m_num_tokens1; ++i)
    if (!m_term1[i] == m_term1[i]) throw i;

  for (size_t i = 0; i < m_num_tokens2; ++i)
    if (!m_term2[i] == m_term2[i]) throw i;

  m_idf1  = reinterpret_cast<float const*>(m_idf1_file.data());
  for (size_t i = 0; i < m_num_tokens1; ++i)
    if (!m_idf1[i] == m_idf1[i]) throw i;
  
  std::ifstream S_in((bname + "lsi.S").c_str());
  m_S.reserve(m_num_dimensions);
  float f;
  while (S_in >> f) m_S.push_back(f);
}

void 
LsaModel::
adapt(boost::unordered_map<uint32_t, uint32_t> const& wcnt, float* dest) const
{
  typedef boost::unordered_map<uint32_t, uint32_t>::const_iterator iter_t;
  iter_t t = wcnt.begin();
  while (t != wcnt.end() && t->second >= m_num_tokens1) ++t;

  float const* f = m_term1 + t->first * m_num_cols;
  float  s = f[m_num_dimensions];
  for (size_t i = 0; i < m_S.size(); ++i)
    {
      float tf_idf = log(1 + t->second) * m_idf1[t->first];
      dest[i] =  tf_idf * (f[i] / m_S[i]) * s;
      //[0] the stored term vectors are length-normalized 
      //[1] row vectors of T * S^{1/2} (with SVD into T S D ), with the original
      //[2] vector length stored in f[m_num_dimensions];
      //[3] thus, (f[i] * s) / m_S[i]^{1/2} is the original cell value of T
      //[4] for fold-in of the document, we divide by T[k][i] by m_S[i], 
      //[5] then multiply again by m_S[i]^{1/2} for computation of the cosine.
      //[6] Notice that 
      //[7] ... / m_S[i]^{1/2} (line 3); and 
      //[8] ... * m_S[i]^{1/2} (line 5) cancel each other out
    }
  for (++t; t != wcnt.end(); ++t)
    {
      if (t->first >= m_num_tokens1) continue;
      f = m_term1 + t->first * m_num_cols;
      s = f[m_num_dimensions];
      for (size_t i = 0; i < m_S.size(); ++i)
        {
          float tf_idf = log(1 + t->second) * m_idf1[t->first];
          dest[i] += tf_idf * (f[i] / m_S[i]) * s;
#if 0
          std::cout << tf_idf << "[tf-idf] " 
                    << f[i]   << "[f] " 
                    << m_S[i] << "[S] " 
                    << s      << "[x] "
                    << dest[i] << " [" << __FILE__ << ":" << __LINE__ << "]"
                    << std::endl;
#endif
        }
    }
  
  float total = 0;
  for (size_t i = 0; i < m_S.size(); ++i)
    total += pow(dest[i],2);
  float denom = sqrt(total);
  if (denom)
    for (size_t i = 0; i < m_S.size(); ++i)
      dest[i] /= denom;
}

void 
LsaModel::
adapt(std::vector<std::string> const& document, float* dest) const
{
  // TO DO: less awful tokenization (use StringPiece!, avoid streams!)
  boost::unordered_map<uint32_t,uint32_t> wcnt;
  std::string word;
  BOOST_FOREACH(std::string const& line, document)
    {
      std::istringstream buf(line);
      while (buf >> word) ++wcnt[V1[word]];
    }
  adapt(wcnt,dest);
}

void
LsaTermMatcher::
init(LsaModel const* model, 
     boost::unordered_map<uint32_t, uint32_t> const& wordcounts)
{
  m_model = model;
  m_document_vector.resize(model->cols());
  model->adapt(wordcounts, &m_document_vector[0]);
}

void
LsaTermMatcher::
init(LsaModel const* model, std::vector<std::string> const& doc)
{
  m_model = model;
  m_document_vector.resize(model->cols());
  model->adapt(doc, &m_document_vector[0]);
}

float
LsaTermMatcher::
operator()(uint32_t const id) const
{
  float const* t = m_model->tvec2(id);
  if (t == NULL) return log(0.5);
  float score = 1;
  for (size_t i = 0; i < m_model->cols(); ++i)
    score += m_document_vector[i] * t[i];
  return log(score/2);
}



}
