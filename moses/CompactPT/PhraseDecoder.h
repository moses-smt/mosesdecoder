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
#include <fstream>
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
#include "moses/UserMessage.h"

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
    
    typedef std::pair<uint8_t, uint8_t> AlignPoint;
    typedef std::pair<uint32_t, uint32_t> SrcTrg;
        
    enum Coding { None, REnc, PREnc } m_coding;
    
    uint64_t m_numScoreComponent;
    bool m_containsAlignmentInfo;
    uint64_t m_maxRank;
    uint64_t m_maxPhraseLength;
    
    boost::unordered_map<std::string, uint32_t> m_sourceSymbolsMap;
    StringVector<unsigned char, uint32_t, std::allocator> m_sourceSymbols;
    StringVector<unsigned char, uint32_t, std::allocator> m_targetSymbols;
    
    std::vector<uint64_t> m_lexicalTableIndex;
    std::vector<SrcTrg> m_lexicalTable;
    
    CanonicalHuffman<uint32_t>* m_symbolTree;
    
    bool m_multipleScoreTrees;
    std::vector<CanonicalHuffman<float>*> m_scoreTrees;
    
    CanonicalHuffman<AlignPoint>* m_alignTree;
    
    TargetPhraseCollectionCache m_decodingCache;
    
    PhraseDictionaryCompact& m_phraseDictionary;   
    
    // ***********************************************
    
    const std::vector<FactorType>* m_input;
    const std::vector<FactorType>* m_output;
    const PhraseDictionaryFeature* m_feature;
    const std::vector<float>* m_weight;
    float m_weightWP;
    const LMList* m_languageModels;
    
    std::string m_separator;
  
    // ***********************************************
    
    unsigned GetSourceSymbolId(std::string& s);
    std::string GetTargetSymbol(uint32_t id) const;
    
    size_t GetREncType(uint32_t encodedSymbol);
    size_t GetPREncType(uint32_t encodedSymbol);
    
    uint32_t GetTranslation(uint32_t srcIdx, size_t rank);
    
    size_t GetMaxSourcePhraseLength();
    
    uint32_t DecodeREncSymbol1(uint32_t encodedSymbol);
    uint32_t DecodeREncSymbol2Rank(uint32_t encodedSymbol);
    uint32_t DecodeREncSymbol2Position(uint32_t encodedSymbol);
    uint32_t DecodeREncSymbol3(uint32_t encodedSymbol);
    
    uint32_t DecodePREncSymbol1(uint32_t encodedSymbol);
    int32_t DecodePREncSymbol2Left(uint32_t encodedSymbol);
    int32_t DecodePREncSymbol2Right(uint32_t encodedSymbol);
    uint32_t DecodePREncSymbol2Rank(uint32_t encodedSymbol);
    
    std::string MakeSourceKey(std::string &);
    
  public:
    
    PhraseDecoder(
      PhraseDictionaryCompact &phraseDictionary,
      const std::vector<FactorType>* &input,
      const std::vector<FactorType>* &output,
      const PhraseDictionaryFeature* feature,
      size_t numScoreComponent,
      const std::vector<float>* weight,
      float weightWP,
      const LMList* languageModels
    );
    
    ~PhraseDecoder();
     
    uint64_t Load(std::FILE* in);
    
    TargetPhraseVectorPtr CreateTargetPhraseCollection(const Phrase &sourcePhrase,
                                                       bool topLevel = false);
    
    TargetPhraseVectorPtr DecodeCollection(TargetPhraseVectorPtr tpv,
                                           BitWrapper<> &encodedBitStream,
                                           const Phrase &sourcePhrase,
                                           bool topLevel);
    
    void PruneCache();
};

}

#endif
