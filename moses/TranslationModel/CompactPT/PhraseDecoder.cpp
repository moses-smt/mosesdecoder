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

#include <deque>

#include "PhraseDecoder.h"
#include "moses/StaticData.h"

using namespace std;

namespace Moses
{

PhraseDecoder::PhraseDecoder(
  PhraseDictionaryCompact &phraseDictionary,
  const std::vector<FactorType>* input,
  const std::vector<FactorType>* output,
  size_t numScoreComponent,
  const std::vector<float>* weight
)
  : m_coding(None), m_numScoreComponent(numScoreComponent),
    m_containsAlignmentInfo(true), m_maxRank(0),
    m_symbolTree(0), m_multipleScoreTrees(false),
    m_scoreTrees(1), m_alignTree(0),
    m_phraseDictionary(phraseDictionary), m_input(input), m_output(output),
    m_weight(weight),
    m_separator(" ||| ")
{ }

PhraseDecoder::~PhraseDecoder()
{
  if(m_symbolTree)
    delete m_symbolTree;

  for(size_t i = 0; i < m_scoreTrees.size(); i++)
    if(m_scoreTrees[i])
      delete m_scoreTrees[i];

  if(m_alignTree)
    delete m_alignTree;
}

inline unsigned PhraseDecoder::GetSourceSymbolId(std::string& symbol)
{
  boost::unordered_map<std::string, unsigned>::iterator it
  = m_sourceSymbolsMap.find(symbol);
  if(it != m_sourceSymbolsMap.end())
    return it->second;

  size_t idx = m_sourceSymbols.find(symbol);
  m_sourceSymbolsMap[symbol] = idx;
  return idx;
}

inline std::string PhraseDecoder::GetTargetSymbol(unsigned idx) const
{
  if(idx < m_targetSymbols.size())
    return m_targetSymbols[idx];
  return std::string("##ERROR##");
}

inline size_t PhraseDecoder::GetREncType(unsigned encodedSymbol)
{
  return (encodedSymbol >> 30) + 1;
}

inline size_t PhraseDecoder::GetPREncType(unsigned encodedSymbol)
{
  return (encodedSymbol >> 31) + 1;
}

inline unsigned PhraseDecoder::GetTranslation(unsigned srcIdx, size_t rank)
{
  size_t srcTrgIdx = m_lexicalTableIndex[srcIdx];
  return m_lexicalTable[srcTrgIdx + rank].second;
}

size_t PhraseDecoder::GetMaxSourcePhraseLength()
{
  return m_maxPhraseLength;
}

inline unsigned PhraseDecoder::DecodeREncSymbol1(unsigned encodedSymbol)
{
  return encodedSymbol &= ~(3 << 30);
}

inline unsigned PhraseDecoder::DecodeREncSymbol2Rank(unsigned encodedSymbol)
{
  return encodedSymbol &= ~(255 << 24);
}

inline unsigned PhraseDecoder::DecodeREncSymbol2Position(unsigned encodedSymbol)
{
  encodedSymbol &= ~(3 << 30);
  encodedSymbol >>= 24;
  return encodedSymbol;
}

inline unsigned PhraseDecoder::DecodeREncSymbol3(unsigned encodedSymbol)
{
  return encodedSymbol &= ~(3 << 30);
}

inline unsigned PhraseDecoder::DecodePREncSymbol1(unsigned encodedSymbol)
{
  return encodedSymbol &= ~(1 << 31);
}

inline int PhraseDecoder::DecodePREncSymbol2Left(unsigned encodedSymbol)
{
  return ((encodedSymbol >> 25) & 63) - 32;
}

inline int PhraseDecoder::DecodePREncSymbol2Right(unsigned encodedSymbol)
{
  return ((encodedSymbol >> 19) & 63) - 32;
}

inline unsigned PhraseDecoder::DecodePREncSymbol2Rank(unsigned encodedSymbol)
{
  return (encodedSymbol & 524287);
}

size_t PhraseDecoder::Load(std::FILE* in)
{
  size_t start = std::ftell(in);
  size_t read = 0;

  read += std::fread(&m_coding, sizeof(m_coding), 1, in);
  read += std::fread(&m_numScoreComponent, sizeof(m_numScoreComponent), 1, in);
  read += std::fread(&m_containsAlignmentInfo, sizeof(m_containsAlignmentInfo), 1, in);
  read += std::fread(&m_maxRank, sizeof(m_maxRank), 1, in);
  read += std::fread(&m_maxPhraseLength, sizeof(m_maxPhraseLength), 1, in);

  if(m_coding == REnc) {
    m_sourceSymbols.load(in);

    size_t size;
    read += std::fread(&size, sizeof(size_t), 1, in);
    m_lexicalTableIndex.resize(size);
    read += std::fread(&m_lexicalTableIndex[0], sizeof(size_t), size, in);

    read += std::fread(&size, sizeof(size_t), 1, in);
    m_lexicalTable.resize(size);
    read += std::fread(&m_lexicalTable[0], sizeof(SrcTrg), size, in);
  }

  m_targetSymbols.load(in);

  m_symbolTree = new CanonicalHuffman<unsigned>(in);

  read += std::fread(&m_multipleScoreTrees, sizeof(m_multipleScoreTrees), 1, in);
  if(m_multipleScoreTrees) {
    m_scoreTrees.resize(m_numScoreComponent);
    for(size_t i = 0; i < m_numScoreComponent; i++)
      m_scoreTrees[i] = new CanonicalHuffman<float>(in);
  } else {
    m_scoreTrees.resize(1);
    m_scoreTrees[0] = new CanonicalHuffman<float>(in);
  }

  if(m_containsAlignmentInfo)
    m_alignTree = new CanonicalHuffman<AlignPoint>(in);

  size_t end = std::ftell(in);
  return end - start;
}

std::string PhraseDecoder::MakeSourceKey(std::string &source)
{
  return source + m_separator;
}

TargetPhraseVectorPtr PhraseDecoder::CreateTargetPhraseCollection(const Phrase &sourcePhrase, bool topLevel, bool eval)
{

  // Not using TargetPhraseCollection avoiding "new" operator
  // which can introduce heavy locking with multiple threads
  TargetPhraseVectorPtr tpv(new TargetPhraseVector());
  size_t bitsLeft = 0;

  if(m_coding == PREnc) {
    std::pair<TargetPhraseVectorPtr, size_t> cachedPhraseColl
    = m_decodingCache.Retrieve(sourcePhrase);

    // Has been cached and is complete or does not need to be completed
    if(cachedPhraseColl.first != NULL && (!topLevel || cachedPhraseColl.second == 0))
      return cachedPhraseColl.first;

    // Has been cached, but is incomplete
    else if(cachedPhraseColl.first != NULL) {
      bitsLeft = cachedPhraseColl.second;
      tpv->resize(cachedPhraseColl.first->size());
      std::copy(cachedPhraseColl.first->begin(),
                cachedPhraseColl.first->end(),
                tpv->begin());
    }
  }

  // Retrieve source phrase identifier
  std::string sourcePhraseString = sourcePhrase.GetStringRep(*m_input);
  size_t sourcePhraseId = m_phraseDictionary.m_hash[MakeSourceKey(sourcePhraseString)];

  if(sourcePhraseId != m_phraseDictionary.m_hash.GetSize()) {
    // Retrieve compressed and encoded target phrase collection
    std::string encodedPhraseCollection;
    if(m_phraseDictionary.m_inMemory)
      encodedPhraseCollection = m_phraseDictionary.m_targetPhrasesMemory[sourcePhraseId];
    else
      encodedPhraseCollection = m_phraseDictionary.m_targetPhrasesMapped[sourcePhraseId];

    BitWrapper<> encodedBitStream(encodedPhraseCollection);
    if(m_coding == PREnc && bitsLeft)
      encodedBitStream.SeekFromEnd(bitsLeft);

    // Decompress and decode target phrase collection
    TargetPhraseVectorPtr decodedPhraseColl =
      DecodeCollection(tpv, encodedBitStream, sourcePhrase, topLevel, eval);

    return decodedPhraseColl;
  } else
    return TargetPhraseVectorPtr();
}

TargetPhraseVectorPtr PhraseDecoder::DecodeCollection(
  TargetPhraseVectorPtr tpv, BitWrapper<> &encodedBitStream,
  const Phrase &sourcePhrase, bool topLevel, bool eval)
{

  bool extending = tpv->size();
  size_t bitsLeft = encodedBitStream.TellFromEnd();

  typedef std::pair<size_t, size_t> AlignPointSizeT;

  std::vector<int> sourceWords;
  if(m_coding == REnc) {
    for(size_t i = 0; i < sourcePhrase.GetSize(); i++) {
      std::string sourceWord
      = sourcePhrase.GetWord(i).GetString(*m_input, false);
      unsigned idx = GetSourceSymbolId(sourceWord);
      sourceWords.push_back(idx);
    }
  }

  unsigned phraseStopSymbol = 0;
  AlignPoint alignStopSymbol(-1, -1);

  std::vector<float> scores;
  std::set<AlignPointSizeT> alignment;

  enum DecodeState { New, Symbol, Score, Alignment, Add } state = New;

  size_t srcSize = sourcePhrase.GetSize();

  TargetPhrase* targetPhrase = NULL;
  while(encodedBitStream.TellFromEnd()) {

    if(state == New) {
      // Creating new TargetPhrase on the heap
      tpv->push_back(TargetPhrase());
      targetPhrase = &tpv->back();

      alignment.clear();
      scores.clear();

      state = Symbol;
    }

    if(state == Symbol) {
      unsigned symbol = m_symbolTree->Read(encodedBitStream);
      if(symbol == phraseStopSymbol) {
        state = Score;
      } else {
        if(m_coding == REnc) {
          std::string wordString;
          size_t type = GetREncType(symbol);

          if(type == 1) {
            unsigned decodedSymbol = DecodeREncSymbol1(symbol);
            wordString = GetTargetSymbol(decodedSymbol);
          } else if (type == 2) {
            size_t rank = DecodeREncSymbol2Rank(symbol);
            size_t srcPos = DecodeREncSymbol2Position(symbol);

            if(srcPos >= sourceWords.size())
              return TargetPhraseVectorPtr();

            wordString = GetTargetSymbol(GetTranslation(sourceWords[srcPos], rank));
            if(m_phraseDictionary.m_useAlignmentInfo) {
              size_t trgPos = targetPhrase->GetSize();
              alignment.insert(AlignPoint(srcPos, trgPos));
            }
          } else if(type == 3) {
            size_t rank = DecodeREncSymbol3(symbol);
            size_t srcPos = targetPhrase->GetSize();

            if(srcPos >= sourceWords.size())
              return TargetPhraseVectorPtr();

            wordString = GetTargetSymbol(GetTranslation(sourceWords[srcPos], rank));
            if(m_phraseDictionary.m_useAlignmentInfo) {
              size_t trgPos = srcPos;
              alignment.insert(AlignPoint(srcPos, trgPos));
            }
          }

          Word word;
          word.CreateFromString(Output, *m_output, wordString, false);
          targetPhrase->AddWord(word);
        } else if(m_coding == PREnc) {
          // if the symbol is just a word
          if(GetPREncType(symbol) == 1) {
            unsigned decodedSymbol = DecodePREncSymbol1(symbol);

            Word word;
            word.CreateFromString(Output, *m_output,
                                  GetTargetSymbol(decodedSymbol), false);
            targetPhrase->AddWord(word);
          }
          // if the symbol is a subphrase pointer
          else {
            int left = DecodePREncSymbol2Left(symbol);
            int right = DecodePREncSymbol2Right(symbol);
            unsigned rank = DecodePREncSymbol2Rank(symbol);

            int srcStart = left + targetPhrase->GetSize();
            int srcEnd   = srcSize - right - 1;

            // false positive consistency check
            if(0 > srcStart || srcStart > srcEnd || unsigned(srcEnd) >= srcSize)
              return TargetPhraseVectorPtr();

            // false positive consistency check
            if(m_maxRank && rank > m_maxRank)
              return TargetPhraseVectorPtr();

            // set subphrase by default to itself
            TargetPhraseVectorPtr subTpv = tpv;

            // if range smaller than source phrase retrieve subphrase
            if(unsigned(srcEnd - srcStart + 1) != srcSize) {
              Phrase subPhrase = sourcePhrase.GetSubString(WordsRange(srcStart, srcEnd));
              subTpv = CreateTargetPhraseCollection(subPhrase, false);
            } else {
              // false positive consistency check
              if(rank >= tpv->size()-1)
                return TargetPhraseVectorPtr();
            }

            // false positive consistency check
            if(subTpv != NULL && rank < subTpv->size()) {
              // insert the subphrase into the main target phrase
              TargetPhrase& subTp = subTpv->at(rank);
              if(m_phraseDictionary.m_useAlignmentInfo) {
                // reconstruct the alignment data based on the alignment of the subphrase
                for(AlignmentInfo::const_iterator it = subTp.GetAlignTerm().begin();
                    it != subTp.GetAlignTerm().end(); it++) {
                  alignment.insert(AlignPointSizeT(srcStart + it->first,
                                                   targetPhrase->GetSize() + it->second));
                }
              }
              targetPhrase->Append(subTp);
            } else
              return TargetPhraseVectorPtr();
          }
        } else {
          Word word;
          word.CreateFromString(Output, *m_output,
                                GetTargetSymbol(symbol), false);
          targetPhrase->AddWord(word);
        }
      }
    } else if(state == Score) {
      size_t idx = m_multipleScoreTrees ? scores.size() : 0;
      float score = m_scoreTrees[idx]->Read(encodedBitStream);
      scores.push_back(score);

      if(scores.size() == m_numScoreComponent) {
        targetPhrase->GetScoreBreakdown().Assign(&m_phraseDictionary, scores);

        if(m_containsAlignmentInfo)
          state = Alignment;
        else
          state = Add;
      }
    } else if(state == Alignment) {
      AlignPoint alignPoint = m_alignTree->Read(encodedBitStream);
      if(alignPoint == alignStopSymbol) {
        state = Add;
      } else {
        if(m_phraseDictionary.m_useAlignmentInfo)
          alignment.insert(AlignPointSizeT(alignPoint));
      }
    }

    if(state == Add) {
      if(m_phraseDictionary.m_useAlignmentInfo) {
        targetPhrase->SetAlignTerm(alignment);
      }

      if(eval) {
        targetPhrase->EvaluateInIsolation(sourcePhrase, m_phraseDictionary.GetFeaturesToApply());
      }

      if(m_coding == PREnc) {
        if(!m_maxRank || tpv->size() <= m_maxRank)
          bitsLeft = encodedBitStream.TellFromEnd();

        if(!topLevel && m_maxRank && tpv->size() >= m_maxRank)
          break;
      }

      if(encodedBitStream.TellFromEnd() <= 8)
        break;

      state = New;
    }
  }

  if(m_coding == PREnc && !extending) {
    bitsLeft = bitsLeft > 8 ? bitsLeft : 0;
    m_decodingCache.Cache(sourcePhrase, tpv, bitsLeft, m_maxRank);
  }

  return tpv;
}

void PhraseDecoder::PruneCache()
{
  m_decodingCache.Prune();
}

}
