// -*- c++ -*-
// vim:tabstop=2
// $Id$
/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include "LexicalReorderingTableCompact.h"
#include "moses/parameters/OOVHandlingOptions.h"

namespace Moses
{
bool LexicalReorderingTableCompact::s_inMemoryByDefault = false;

LexicalReorderingTableCompact::
LexicalReorderingTableCompact(const std::string& filePath,
                              const std::vector<FactorType>& f_factors,
                              const std::vector<FactorType>& e_factors,
                              const std::vector<FactorType>& c_factors)
  : LexicalReorderingTable(f_factors, e_factors, c_factors)
  , m_inMemory(s_inMemoryByDefault)
  , m_numScoreComponent(6)
  , m_multipleScoreTrees(true)
  , m_hash(10, 16)
  , m_scoreTrees(1)
{
  Load(filePath);
}

LexicalReorderingTableCompact::
LexicalReorderingTableCompact(const std::vector<FactorType>& f_factors,
                              const std::vector<FactorType>& e_factors,
                              const std::vector<FactorType>& c_factors)
  : LexicalReorderingTable(f_factors, e_factors, c_factors)
  , m_inMemory(s_inMemoryByDefault)
  , m_numScoreComponent(6)
  , m_multipleScoreTrees(true)
  , m_hash(10, 16)
  , m_scoreTrees(1)
{ }

LexicalReorderingTableCompact::
~LexicalReorderingTableCompact()
{
  for(size_t i = 0; i < m_scoreTrees.size(); i++)
    delete m_scoreTrees[i];
}

std::vector<float>
LexicalReorderingTableCompact::
GetScore(const Phrase& f, const Phrase& e, const Phrase& c)
{
  std::string key;
  Scores scores;

  if(0 == c.GetSize())
    key = MakeKey(f, e, c);
  else
    for(size_t i = 0; i <= c.GetSize(); ++i) {
      Phrase sub_c(c.GetSubString(Range(i,c.GetSize()-1)));
      key = MakeKey(f,e,sub_c);
    }

  size_t index = m_hash[key];
  if(m_hash.GetSize() != index) {
    std::string scoresString;
    if(m_inMemory)
      scoresString = m_scoresMemory[index].str();
    else
      scoresString = m_scoresMapped[index].str();

    BitWrapper<> bitStream(scoresString);
    for(size_t i = 0; i < m_numScoreComponent; i++)
      scores.push_back(m_scoreTrees[m_multipleScoreTrees ? i : 0]->Read(bitStream));

    return scores;
  }

  return Scores();
}

std::string
LexicalReorderingTableCompact::
MakeKey(const Phrase& f,
        const Phrase& e,
        const Phrase& c) const
{
  return MakeKey(Trim(f.GetStringRep(m_FactorsF)),
                 Trim(e.GetStringRep(m_FactorsE)),
                 Trim(c.GetStringRep(m_FactorsC)));
}

std::string
LexicalReorderingTableCompact::
MakeKey(const std::string& f,
        const std::string& e,
        const std::string& c) const
{
  std::string key;
  if(!f.empty()) key += f;
  if(!m_FactorsE.empty()) {
    if(!key.empty()) key += " ||| ";
    key += e;
  }
  if(!m_FactorsC.empty()) {
    if(!key.empty()) key += " ||| ";
    key += c;
  }
  key += " ||| ";
  return key;
}

LexicalReorderingTable*
LexicalReorderingTableCompact::
CheckAndLoad
(const std::string& filePath,
 const std::vector<FactorType>& f_factors,
 const std::vector<FactorType>& e_factors,
 const std::vector<FactorType>& c_factors)
{
#ifdef HAVE_CMPH
  std::string minlexr = ".minlexr";
  // file name is specified without suffix
  if(FileExists(filePath + minlexr)) {
    //there exists a compact binary version use that
    VERBOSE(2,"Using compact lexical reordering table" << std::endl);
    return new LexicalReorderingTableCompact(filePath + minlexr, f_factors, e_factors, c_factors);
  }
  // file name is specified with suffix
  if(filePath.substr(filePath.length() - minlexr.length(), minlexr.length()) == minlexr
      && FileExists(filePath)) {
    //there exists a compact binary version use that
    VERBOSE(2,"Using compact lexical reordering table" << std::endl);
    return new LexicalReorderingTableCompact(filePath, f_factors, e_factors, c_factors);
  }
#endif
  return 0;
}

void
LexicalReorderingTableCompact::
Load(std::string filePath)
{
  std::FILE* pFile = std::fopen(filePath.c_str(), "r");
  UTIL_THROW_IF2(pFile == NULL, "File " << filePath << " could not be opened");

  //if(m_inMemory)
  m_hash.Load(pFile);
  //else
  //m_hash.LoadIndex(pFile);

  size_t read = 0;
  read += std::fread(&m_numScoreComponent, sizeof(m_numScoreComponent), 1, pFile);
  read += std::fread(&m_multipleScoreTrees,
                     sizeof(m_multipleScoreTrees), 1, pFile);

  if(m_multipleScoreTrees) {
    m_scoreTrees.resize(m_numScoreComponent);
    for(size_t i = 0; i < m_numScoreComponent; i++)
      m_scoreTrees[i] = new CanonicalHuffman<float>(pFile);
  } else {
    m_scoreTrees.resize(1);
    m_scoreTrees[0] = new CanonicalHuffman<float>(pFile);
  }

  if(m_inMemory)
    m_scoresMemory.load(pFile, false);
  else
    m_scoresMapped.load(pFile, true);
}

void
LexicalReorderingTableCompact::
SetStaticDefaultParameters(Parameter const& param)
{
  param.SetParameter(s_inMemoryByDefault, "minlexr-memory", false);
}


}
