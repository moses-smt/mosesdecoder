// $Id: LanguageModelIRST.cpp 3719 2010-11-17 14:06:21Z chardmeier $

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
  delete m_lmtb_ng;
}


bool LanguageModelIRST::Load(const std::string &filePath, 
			     FactorType factorType, 
			     size_t nGramOrder)
{
  const char *SepString = " \t\n";
  cerr << "In LanguageModelIRST::Load: nGramOrder = " << nGramOrder << "\n";

  FactorCollection &factorCollection = FactorCollection::Instance();

  m_factorType 	 = factorType;
  m_nGramOrder	 = nGramOrder;

  // get name of LM file and, if any, of the micro-macro map file
  char *filenamesOrig = strdup(filePath.c_str());
  char *filenames = filenamesOrig;
  m_filePath = strsep(&filenames, SepString);

  // Open the input file (possibly gzipped)
  InputFileStream inp(m_filePath);

  if (filenames) {
    // case LMfile + MAPfile: create an object of lmmacro class and load both LM file and map
    cerr << "Loading LM file + MAP\n";
    m_mapFilePath = strsep(&filenames, SepString);
    if (!FileExists(m_mapFilePath)) {
      cerr << "ERROR: Map file <" << m_mapFilePath << "> does not exist\n";
			free(filenamesOrig);
      return false;
    }
    InputFileStream inpMap(m_mapFilePath);
    m_lmtb = new lmmacro(m_filePath, inp, inpMap);


  } else {
    // case (standard) LMfile only: create an object of lmtable
    cerr << "Loading LM file (no MAP)\n";
    m_lmtb  = (lmtable *)new lmtable;

  // Load the (possibly binary) model
#ifdef WIN32
    m_lmtb->load(inp); //don't use memory map
#else
    if (m_filePath.compare(m_filePath.size()-3,3,".mm")==0)
      m_lmtb->load(inp,m_filePath.c_str(),NULL,1);
    else 
      m_lmtb->load(inp,m_filePath.c_str(),NULL,0);
#endif  

  }

  m_lmtb_ng=new ngram(m_lmtb->getDict()); // ngram of words/micro tags
  m_lmtb_size=m_lmtb->maxlevel();

  // LM can be ok, just outputs warnings

  // Mauro: in the original, the following two instructions are wrongly switched:
  m_unknownId = m_lmtb->getDict()->oovcode(); // at the level of micro tags
  CreateFactors(factorCollection);

  VERBOSE(0, "IRST: m_unknownId=" << m_unknownId << std::endl);

  //install caches
  m_lmtb->init_caches(m_lmtb->maxlevel()>2?m_lmtb->maxlevel()-1:2);

  if (m_lmtb_dub >0) m_lmtb->setlogOOVpenalty(m_lmtb_dub);

  free(filenamesOrig);
  return true;
}

void LanguageModelIRST::CreateFactors(FactorCollection &factorCollection)
{ // add factors which have srilm id
	// code copied & paste from SRI LM class. should do template function
	std::map<size_t, int> lmIdMap;
	size_t maxFactorId = 0; // to create lookup vector later on
	
	dict_entry *entry;
	dictionary_iter iter(m_lmtb->getDict()); // at the level of micro tags
	while ( (entry = iter.next()) != NULL)
	{
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
	for (iterMap = lmIdMap.begin() ; iterMap != lmIdMap.end() ; ++iterMap)
	{
		m_lmIdLookup[iterMap->first] = iterMap->second;
	}
  
  
}

int LanguageModelIRST::GetLmID( const std::string &str ) const
{
  return m_lmtb->getDict()->encode( str.c_str() ); // at the level of micro tags
}

int LanguageModelIRST::GetLmID( const Factor *factor ) const
{
        size_t factorId = factor->GetId();
        return ( factorId >= m_lmIdLookup.size()) ? m_unknownId : m_lmIdLookup[factorId];
}

float LanguageModelIRST::GetValue(const vector<const Word*> &contextFactor, State* finalState, unsigned int* len) const
{
	unsigned int dummy;
	if (!len) { len = &dummy; }
	FactorType factorType = GetFactorType();

	// set up context
	size_t count = contextFactor.size();
        if (count < 0) { cerr << "ERROR count < 0\n"; exit(100); };

        // set up context
        int codes[MAX_NGRAM_SIZE];

	size_t idx=0;
        //fill the farthest positions with at most ONE sentenceEnd symbol and at most ONE sentenceEnd symbol, if "empty" positions are available
	//so that the vector looks like = "</s> <s> context_word context_word" for a two-word context and a LM of order 5
	if (count < (size_t) (m_lmtb_size-1)) codes[idx++] = m_lmtb_sentenceEnd;  
	if (count < (size_t) m_lmtb_size) codes[idx++] = m_lmtb_sentenceStart;  

        for (size_t i = 0 ; i < count ; i++)
                codes[idx++] =  GetLmID((*contextFactor[i])[factorType]);

        float prob;
        char* msp = NULL;
        unsigned int ilen;
        prob = m_lmtb->clprob(codes,idx,NULL,NULL,&msp,&ilen);

	if (finalState) *finalState=(State *) msp;
	if (len) *len = ilen;

	return TransformLMScore(prob);
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
	
	if (LMCacheCleanup(sentenceCount, lmcache_cleanup_threshold)){
		TRACE_ERR( "reset caches\n");
		m_lmtb->reset_caches(); 
	}
}

void LanguageModelIRST::InitializeBeforeSentenceProcessing(){
  //nothing to do
#ifdef TRACE_CACHE
 m_lmtb->sentence_id++;
#endif
}

}

