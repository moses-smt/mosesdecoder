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

#include <cassert>
#include <limits>
#include <iostream>
#include <fstream>
#include "dictionary.h"
#include "n_gram.h"
#include "lmtable.h"
#include "lmmacro.h"
#include "lmclass.h"


#include "LanguageModelIRST.h"
#include "TypeDef.h"
#include "Util.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "InputFileStream.h"
#include "StaticData.h"

using namespace std;

namespace Moses
{

LanguageModelIRST::LanguageModelIRST(int dub)
  :m_lmtb(0),m_lmtb_dub(dub)
{
}

LanguageModelIRST::~LanguageModelIRST()
{

#ifndef WIN32
  TRACE_ERR( "reset mmap\n");
  m_lmtb->reset_mmap();
#endif

  delete m_lmtb;
}


bool LanguageModelIRST::Load(const std::string &filePath,
                             FactorType factorType,
                             size_t nGramOrder)
{
  cerr << "In LanguageModelIRST::Load: nGramOrder = " << nGramOrder << "\n";

  FactorCollection &factorCollection = FactorCollection::Instance();

  m_factorType 	 = factorType;
  m_nGramOrder	 = nGramOrder;
  m_filePath = filePath;

  //checking the language model type
  int lmtype = getLanguageModelType(m_filePath);
  std::cerr << "IRSTLM Language Model Type of " << filePath << " is " << lmtype << std::endl;

  if (lmtype == _IRSTLM_LMMACRO) {
    // case lmmacro: LM is of type lmmacro, create an object of lmmacro

    m_lmtb = new lmmacro();
    d=((lmmacro *)m_lmtb)->getDict();

    ((lmmacro*) m_lmtb)->load(m_filePath);
  } else if (lmtype == _IRSTLM_LMCLASS) {
    // case lmclass: LM is of type lmclass, create an object of lmclass

    m_lmtb = new lmclass();
    d=((lmclass *)m_lmtb)->getDict();

    ((lmclass*) m_lmtb)->load(m_filePath);
  } else if (lmtype == _IRSTLM_LMTABLE) {
    // case (standard) lmmacro: LM is of type lmtable: create an object of lmtable
    std::cerr << "Loading LM file (no MAP)\n";
    m_lmtb  = (lmtable *)new lmtable();
    d=((lmtable *)m_lmtb)->getDict();

    // Load the (possibly binary) model
    // Open the input file (possibly gzipped)
    InputFileStream inp(m_filePath);

#ifdef WIN32
    m_lmtb->load(inp); //don't use memory map
#else
    if (m_filePath.compare(m_filePath.size()-3,3,".mm")==0)
      m_lmtb->load(inp,m_filePath.c_str(),NULL,1);
    else
      m_lmtb->load(inp);
#endif

  } else {
    std::cerr << "This language model type is unknown!" << std::endl;
    exit(1);
  }

  if ((lmtype == _IRSTLM_LMMACRO) || ((lmtype == _IRSTLM_LMCLASS))){
    m_lmtb->getDict()->incflag(1);
  }

  m_lmtb_size=m_lmtb->maxlevel();

  // LM can be ok, just outputs warnings

  // Mauro: in the original, the following two instructions are wrongly switched:
  m_unknownId = d->oovcode(); // at the level of micro tags
  CreateFactors(factorCollection);

  VERBOSE(1, "IRST: m_unknownId=" << m_unknownId << std::endl);

  //install caches to save time (only if PS_CACHE_ENABLE is defined through compilation flags)
  int ml = ((lmtable *)m_lmtb)->maxlevel();
  m_lmtb->init_caches(ml>2?ml-1:2);

  if (m_lmtb_dub > 0) m_lmtb->setlogOOVpenalty(m_lmtb_dub);

  return true;
}

void LanguageModelIRST::CreateFactors(FactorCollection &factorCollection)
{
  // add factors which have srilm id
  // code copied & paste from SRI LM class. should do template function
  std::map<size_t, int> lmIdMap;
  size_t maxFactorId = 0; // to create lookup vector later on

  dict_entry *entry;
  dictionary_iter iter(d); // at the level of micro tags
  while ( (entry = iter.next()) != NULL) {
    size_t factorId = factorCollection.AddFactor(Output, m_factorType, entry->word)->GetId();
    lmIdMap[factorId] = entry->code;
    maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
  }

  size_t factorId;

  m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, BOS_);
  factorId = m_sentenceStart->GetId();
  m_lmtb_sentenceStart=lmIdMap[factorId] = GetLmID(BOS_);
  maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
  m_sentenceStartArray[m_factorType] = m_sentenceStart;

  m_sentenceEnd		= factorCollection.AddFactor(Output, m_factorType, EOS_);
  factorId = m_sentenceEnd->GetId();
  m_lmtb_sentenceEnd=lmIdMap[factorId] = GetLmID(EOS_);
  maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
  m_sentenceEndArray[m_factorType] = m_sentenceEnd;

  // add to lookup vector in object
  m_lmIdLookup.resize(maxFactorId+1);

  fill(m_lmIdLookup.begin(), m_lmIdLookup.end(), m_unknownId);

  map<size_t, int>::iterator iterMap;
  for (iterMap = lmIdMap.begin() ; iterMap != lmIdMap.end() ; ++iterMap) {
    m_lmIdLookup[iterMap->first] = iterMap->second;
  }


}

int LanguageModelIRST::GetLmID( const std::string &str ) const
{
  return d->encode( str.c_str() ); // at the level of micro tags
}

int LanguageModelIRST::GetLmID( const Factor *factor ) const
{
  size_t factorId = factor->GetId();

  if  (factorId >= m_lmIdLookup.size()) {
    if (d->incflag()==1) {
      std::string s = factor->GetString();
      int code = d->encode(s.c_str());

////////////
/// IL PPROBLEMA ERA QUI
/// m_lmIdLookup.push_back(code);
/// PERCHE' USANDO PUSH_BACK IN REALTA' INSEREVIVAMO L'ELEMENTO NUOVO
/// IN POSIZIONE (factorID-1) invece che in posizione factrID dove dopo andiamo a leggerlo (vedi caso C
/// Cosi' funziona ....
/// ho un dubbio su cosa c'e' nelle prime posizioni di m_lmIdLookup
/// quindi controllo
/// e scopro che rimane vuota una entry ogni due
/// perche' factorID cresce di due in due (perche' codifica sia source che target) "vuota" la posizione (factorID-1)
/// non da problemi di correttezza, ma solo di "spreco" di memoria
/// potremmo sostituirerendere  m_lmIdLookup una std:map invece che un std::vector,
/// ma si perde in efficienza nell'accesso perche' non e' piu' possibile quello random dei vettori
/// a te la scelta!!!!
////////////////


      //resize and fill with m_unknownId
      m_lmIdLookup.resize(factorId+1, m_unknownId);

      //insert new code
      m_lmIdLookup[factorId] = code;

      return code;

    } else {
      return m_unknownId;
    }
  } else {
    return m_lmIdLookup[factorId];
  }
}

LMResult LanguageModelIRST::GetValue(const vector<const Word*> &contextFactor, State* finalState) const
{
  FactorType factorType = GetFactorType();

  // set up context
  size_t count = contextFactor.size();
  if (count < 0) {
    cerr << "ERROR count < 0\n";
    exit(100);
  };

  // set up context
  int codes[MAX_NGRAM_SIZE];

  size_t idx=0;
  //fill the farthest positions with at most ONE sentenceEnd symbol and at most ONE sentenceEnd symbol, if "empty" positions are available
  //so that the vector looks like = "</s> <s> context_word context_word" for a two-word context and a LM of order 5
  if (count < (size_t) (m_lmtb_size-1)) codes[idx++] = m_lmtb_sentenceEnd;
  if (count < (size_t) m_lmtb_size) codes[idx++] = m_lmtb_sentenceStart;

  for (size_t i = 0 ; i < count ; i++) {
    codes[idx++] =  GetLmID((*contextFactor[i])[factorType]);
  }
  LMResult result;
  result.unknown = (codes[idx - 1] == m_unknownId);

  char* msp = NULL;
  unsigned int ilen;
  result.score = m_lmtb->clprob(codes,idx,NULL,NULL,&msp,&ilen);

  if (finalState) *finalState=(State *) msp;

  result.score = TransformLMScore(result.score);
  return result;
}


bool LMCacheCleanup(size_t sentences_done, size_t m_lmcache_cleanup_threshold)
{
  if (sentences_done==-1) return true;
  if (m_lmcache_cleanup_threshold)
    if (sentences_done % m_lmcache_cleanup_threshold == 0)
      return true;
  return false;
}


void LanguageModelIRST::CleanUpAfterSentenceProcessing()
{
  const StaticData &staticData = StaticData::Instance();
  static int sentenceCount = 0;
  sentenceCount++;

  size_t lmcache_cleanup_threshold = staticData.GetLMCacheCleanupThreshold();

  if (LMCacheCleanup(sentenceCount, lmcache_cleanup_threshold)) {
    TRACE_ERR( "reset caches\n");
    m_lmtb->reset_caches();
  }
}

void LanguageModelIRST::InitializeBeforeSentenceProcessing()
{
  //nothing to do
#ifdef TRACE_CACHE
  m_lmtb->sentence_id++;
#endif
}

}

