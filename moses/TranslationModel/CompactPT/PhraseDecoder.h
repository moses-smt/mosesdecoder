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

#ifndef moses_PhraseDecoder_h
#define moses_PhraseDecoder_h

#include <sstream>
#include <vector>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <string>
#include <iterator>
#include <algorithm>
#include <sys/stat.h>

#include "moses/TypeDef.h"
#include "moses/FactorCollection.h"
#include "moses/Word.h"
#include "moses/Util.h"
#include "moses/InputFileStream.h"
#include "moses/StaticData.h"
#include "moses/WordsRange.h"

#include "PhraseDictionaryCompact.h"
#include "StringVector.h"
#include "CanonicalHuffman.h"
#include "TargetPhraseCollectionCache.h"

namespace Moses
{

class PhraseDictionaryCompact;

class PhraseDecoder
{
protected:

  friend class PhraseDictionaryCompact;

  typedef std::pair<unsigned char, unsigned char> AlignPoint;
  typedef std::pair<unsigned, unsigned> SrcTrg;

  enum Coding { None, REnc, PREnc } m_coding;

  size_t m_numScoreComponent;
  bool m_containsAlignmentInfo;
  size_t m_maxRank;
  size_t m_maxPhraseLength;

  boost::unordered_map<std::string, unsigned> m_sourceSymbolsMap;
  StringVector<unsigned char, unsigned, std::allocator> m_sourceSymbols;
  StringVector<unsigned char, unsigned, std::allocator> m_targetSymbols;

  std::vector<size_t> m_lexicalTableIndex;
  std::vector<SrcTrg> m_lexicalTable;

  CanonicalHuffman<unsigned>* m_symbolTree;

  bool m_multipleScoreTrees;
  std::vector<CanonicalHuffman<float>*> m_scoreTrees;

  CanonicalHuffman<AlignPoint>* m_alignTree;

  TargetPhraseCollectionCache m_decodingCache;

  PhraseDictionaryCompact& m_phraseDictionary;

  // ***********************************************

  const std::vector<FactorType>* m_input;
  const std::vector<FactorType>* m_output;
  const std::vector<float>* m_weight;

  std::string m_separator;

  // ***********************************************

  unsigned GetSourceSymbolId(std::string& s);
  std::string GetTargetSymbol(unsigned id) const;

  size_t GetREncType(unsigned encodedSymbol);
  size_t GetPREncType(unsigned encodedSymbol);

  unsigned GetTranslation(unsigned srcIdx, size_t rank);

  size_t GetMaxSourcePhraseLength();

  unsigned DecodeREncSymbol1(unsigned encodedSymbol);
  unsigned DecodeREncSymbol2Rank(unsigned encodedSymbol);
  unsigned DecodeREncSymbol2Position(unsigned encodedSymbol);
  unsigned DecodeREncSymbol3(unsigned encodedSymbol);

  unsigned DecodePREncSymbol1(unsigned encodedSymbol);
  int DecodePREncSymbol2Left(unsigned encodedSymbol);
  int DecodePREncSymbol2Right(unsigned encodedSymbol);
  unsigned DecodePREncSymbol2Rank(unsigned encodedSymbol);

  std::string MakeSourceKey(std::string &);

public:

  PhraseDecoder(
    PhraseDictionaryCompact &phraseDictionary,
    const std::vector<FactorType>* input,
    const std::vector<FactorType>* output,
    size_t numScoreComponent,
    const std::vector<float>* weight
  );

  ~PhraseDecoder();

  size_t Load(std::FILE* in);

  TargetPhraseVectorPtr CreateTargetPhraseCollection(const Phrase &sourcePhrase,
      bool topLevel = false, bool eval = true);

  TargetPhraseVectorPtr DecodeCollection(TargetPhraseVectorPtr tpv,
                                         BitWrapper<> &encodedBitStream,
                                         const Phrase &sourcePhrase,
                                         bool topLevel,
                                         bool eval);

  void PruneCache();
};

}

#endif
