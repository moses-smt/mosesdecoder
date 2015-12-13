// $Id$
// vim:tabstop=2
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

#ifndef moses_LexicalReorderingTableCompact_h
#define moses_LexicalReorderingTableCompact_h

#include "moses/FF/LexicalReordering/LexicalReorderingTable.h"
#include "moses/StaticData.h"
#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/GenerationDictionary.h"
#include "moses/TargetPhrase.h"
#include "moses/TargetPhraseCollection.h"

#include "BlockHashIndex.h"
#include "CanonicalHuffman.h"
#include "StringVector.h"

namespace Moses
{

class LexicalReorderingTableCompact:
  public LexicalReorderingTable
{
private:
  static bool s_inMemoryByDefault;
  bool m_inMemory;

  size_t m_numScoreComponent;
  bool m_multipleScoreTrees;

  BlockHashIndex m_hash;

  typedef CanonicalHuffman<float> ScoreTree;
  std::vector<ScoreTree*> m_scoreTrees;

  StringVector<unsigned char, unsigned long, MmapAllocator>  m_scoresMapped;
  StringVector<unsigned char, unsigned long, std::allocator> m_scoresMemory;

  std::string MakeKey(const Phrase& f, const Phrase& e, const Phrase& c) const;
  std::string MakeKey(const std::string& f, const std::string& e, const std::string& c) const;

public:
  LexicalReorderingTableCompact(const std::string& filePath,
                                const std::vector<FactorType>& f_factors,
                                const std::vector<FactorType>& e_factors,
                                const std::vector<FactorType>& c_factors);

  LexicalReorderingTableCompact(const std::vector<FactorType>& f_factors,
                                const std::vector<FactorType>& e_factors,
                                const std::vector<FactorType>& c_factors);

  virtual
  ~LexicalReorderingTableCompact();

  virtual
  std::vector<float>
  GetScore(const Phrase& f, const Phrase& e, const Phrase& c);

  static
  LexicalReorderingTable*
  CheckAndLoad(const std::string& filePath,
               const std::vector<FactorType>& f_factors,
               const std::vector<FactorType>& e_factors,
               const std::vector<FactorType>& c_factors);

  void
  Load(std::string filePath);

  static void
  SetStaticDefaultParameters(Parameter const& param);

};

}

#endif
