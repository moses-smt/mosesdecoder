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

#include <limits>
#include <iostream>
#include <fstream>
#include "dictionary.h"
#include "n_gram.h"
#include "lmContainer.h"

using namespace irstlm;

#include "IRST.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/FactorCollection.h"
#include "moses/Phrase.h"
#include "moses/InputFileStream.h"
#include "moses/StaticData.h"

using namespace std;

namespace Moses
{

LanguageModelIRST::LanguageModelIRST(const std::string &line)
  :LanguageModelSingleFactor(line)
  ,m_lmtb_dub(0)
{
  const StaticData &staticData = StaticData::Instance();
  int threadCount = staticData.ThreadCount();
  if (threadCount != 1) {
    throw runtime_error("Error: " + SPrint(threadCount) + " number of threads specified but IRST LM is not threadsafe.");
  }

  ReadParameters();

}

LanguageModelIRST::~LanguageModelIRST()
{

#ifndef WIN32
  TRACE_ERR( "reset mmap\n");
  if (m_lmtb) m_lmtb->reset_mmap();
#endif

  delete m_lmtb;
}


void LanguageModelIRST::Load()
{
  cerr << "In LanguageModelIRST::Load: nGramOrder = " << m_nGramOrder << "\n";

  FactorCollection &factorCollection = FactorCollection::Instance();

  m_lmtb = m_lmtb->CreateLanguageModel(m_filePath);
  m_lmtb->setMaxLoadedLevel(1000);
  m_lmtb->load(m_filePath);
  d=m_lmtb->getDict();
  d->incflag(1);

  m_lmtb_size=m_lmtb->maxlevel();

  // LM can be ok, just outputs warnings
  // Mauro: in the original, the following two instructions are wrongly switched:
  m_unknownId = d->oovcode(); // at the level of micro tags
  m_empty = -1; // code for an empty position

  CreateFactors(factorCollection);

  VERBOSE(1, "IRST: m_unknownId=" << m_unknownId << std::endl);

  //install caches to save time (only if PS_CACHE_ENABLE is defined through compilation flags)
  m_lmtb->init_caches(m_lmtb_size>2?m_lmtb_size-1:2);

  if (m_lmtb_dub > 0) m_lmtb->setlogOOVpenalty(m_lmtb_dub);
}

void LanguageModelIRST::CreateFactors(FactorCollection &factorCollection)
{
  // add factors which have srilm id
  // code copied & paste from SRI LM class. should do template function
  std::map<size_t, int> lmIdMap;
  size_t maxFactorId = 0; // to create lookup vector later on
  m_empty = -1; // code for an empty position

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
  m_sentenceStartWord[m_factorType] = m_sentenceStart;

  m_sentenceEnd		= factorCollection.AddFactor(Output, m_factorType, EOS_);
  factorId = m_sentenceEnd->GetId();
  m_lmtb_sentenceEnd=lmIdMap[factorId] = GetLmID(EOS_);
  maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
  m_sentenceEndWord[m_factorType] = m_sentenceEnd;

  // add to lookup vector in object
  m_lmIdLookup.resize(maxFactorId+1);
  fill(m_lmIdLookup.begin(), m_lmIdLookup.end(), m_empty);

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

  if  ((factorId >= m_lmIdLookup.size()) || (m_lmIdLookup[factorId] == m_empty)) {
    if (d->incflag()==1) {
      std::string s = factor->GetString().as_string();
      int code = d->encode(s.c_str());

      //////////
      ///poiche' non c'e' distinzione tra i factorIDs delle parole sorgenti
      ///e delle parole target in Moses, puo' accadere che una parola target
      ///di cui non sia stato ancora calcolato il suo codice target abbia
      ///comunque un factorID noto (e quindi minore di m_lmIdLookup.size())
      ///E' necessario dunque identificare questi casi di indeterminatezza
      ///del codice target. Attualamente, questo controllo e' stato implementato
      ///impostando a    m_empty     tutti i termini che non hanno ancora
      //ricevuto un codice target effettivo
      ///////////

      ///OLD PROBLEM - SOLVED
////////////
/// IL PPROBLEMA ERA QUI
/// m_lmIdLookup.push_back(code);
/// PERCHE' USANDO PUSH_BACK IN REALTA' INSEREVIVAMO L'ELEMENTO NUOVO
/// IN POSIZIONE (factorID-1) invece che in posizione factrID dove dopo andiamo a leggerlo (vedi caso C
/// Cosi' funziona ....
/// ho un dubbio su cosa c'e' nelle prime posizioni di m_lmIdLookup
/// quindi
/// e scopro che rimane vuota una entry ogni due
/// perche' factorID cresce di due in due (perche' codifica sia source che target) "vuota" la posizione (factorID-1)
/// non da problemi di correttezza, ma solo di "spreco" di memoria
/// potremmo sostituirerendere  m_lmIdLookup una std:map invece che un std::vector,
/// ma si perde in efficienza nell'accesso perche' non e' piu' possibile quello random dei vettori
/// a te la scelta!!!!
////////////////


      if (factorId >= m_lmIdLookup.size()) {
        //resize and fill with m_empty
        //increment the array more than needed to avoid too many resizing operation.
        m_lmIdLookup.resize(factorId+10, m_empty);
      }

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

void LanguageModelIRST::InitializeForInput(InputType const& source)
{
  //nothing to do
#ifdef TRACE_CACHE
  m_lmtb->sentence_id++;
#endif
}

void LanguageModelIRST::CleanUpAfterSentenceProcessing(const InputType& source)
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

}

