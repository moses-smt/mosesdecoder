// $Id: PhraseDictionaryNodeMBOT.h,v 1.1.1.1 2013/01/06 16:54:17 braunefe Exp $
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

#pragma once

#include <map>
#include <vector>
#include <iterator>
#include <utility>
#include <ostream>
#include "Word.h"
#include "TargetPhraseCollection.h"
#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <boost/version.hpp>
#include <boost/tuple/tuple.hpp>
#include "PhraseDictionaryNodeSCFG.h"
#include "MBOTWord.h"

namespace Moses
{

class PhraseDictionarySCFG;
//new : add class PhraseDictionaryMBOT for getting friends
class PhraseDictionaryMBOT;

/*class TerminalHasherMBOT
{
public:
  // Generate a hash value for a word representing a terminal.  It's
  // assumed that the same subset of factors will be active for all words
  // that are hashed.
 //size_t operator()(const std::pair<Word, Word> k1) const {
 size_t operator()(const Word k) const {
     //std::cout << "USING TERMINAL HASHER" << std::endl;
    size_t seed = 0;
    for (size_t i = 0; i < MAX_NUM_FACTORS; ++i) {
      const Factor * f = k[i];
      if (f) {
        boost::hash_combine(seed, *f);
      }
      /*for (size_t i = 0; i < MAX_NUM_FACTORS; ++i) {
      const Factor * f2 = k1.second[i];
      if (f2) {
        boost::hash_combine(seed, *f2);
      }*/
    /*}
    return seed;
  }
};

class TerminalEqualityPredMBOT
{
public:
  // Equality predicate for comparing words representing terminals.  As
  // with the hasher, it's assumed that all words will have the same
  // subset of active factors.
  //BEWARE : WE TAKE
  //bool operator()(const std::pair<Word, Word> k1,
                  //const std::pair<Word, Word> k2) const {
                    //{
bool operator()(const Word w1, const Word w2) const {

    //std::cout << "USING TERMINAL EQUALITY PRED" << std::endl;

      //const Word & w1 = k1.first;
      //std::cout << "First Word : " << w1 << std::endl;
      //const Word & w2 = k2.first;
      //std::cout << "Second Word : " << w2 << std::endl;
      const Factor * f1 = w1[0];
      //std::cout << "First factor : " << *f1 << std::endl;
      const Factor * f2 = w2[0];
       //std::cout << "Second factor : " << *f2 << std::endl;
      //note : factor comparison : if same return 0
      if (f1->Compare(*f2)) {
        //std::cout << "Source Words are not the same !!!" << std::endl;
        return false;
      }
    /*{
        // Compare second non-terminal of each key.
        const Word & w1 = k1.second;
        const Word & w2 = k2.second;

         const Factor * f1 = w1[0];
        std::cout << "First factor : " << *f1 << std::endl;
        const Factor * f2 = w2[0];
        std::cout << "Second factor : " << *f2 << std::endl;

      //note : factor comparison : if same return 0
        if (f1->Compare(*f2)) {
            std::cout << "Source Words are not the same !!!" << std::endl;
            return false;
        }*/
    /*return true;
  }
};


  //helper function for building tuples out of a vector
  /*bool operator()(const Word & t1, const Word & t2) const {
    std::cout << "USING TERMINAL EQUALITY PRED" << std::endl;
    for (size_t i = 0; i < MAX_NUM_FACTORS; ++i) {
      const Factor * f1 = t1[i];
      const Factor * f2 = t2[i];
      if (f1 && f1->Compare(*f2)) {
        return false;
      }
    }
    return true;
  }*/



/*class NonTerminalMapKeyHasherMBOT
{
public:
  size_t operator()(const std::pair<Word, MBOTWord> k) const {
    std::cout << "USING NON TERMINAL KEY HASHER" << std::endl;
    // Assumes that only the first factor of each Word is relevant.
    const Word & w1 = k.first;

    std::cout << "Found first word : " << w1 << std::endl;

    const MBOTWord & w2 = k.second;

    const Factor * f1 = w1[0];

    size_t seed = 0;
    boost::hash_combine(seed, *f1);

    //std::cout << "GETTING TOKEN FROM HASHER"<< std::endl;

    const std::vector<const Factor*> factors = w2.GetFactors();
    //std::cout << "FACTORS GETTED" << std::endl;
    std::vector<const Factor*> :: const_iterator itr_facts;
    for(itr_facts = factors.begin(); itr_facts != factors.end(); itr_facts++)
    {
          //std::cout << "GETTING FACTOR"<< std::endl;
          const Factor * f2 = *itr_facts;
          std::cout << "FACTOR " << *f2 << std::endl;
          boost::hash_combine(seed, *f2);
          //std::cout << "SEED COMBINED"<< std::endl;
    }
    std::cout << "OUT OF HASHER" << std::endl;
    return seed;
  }
};

class NonTerminalMapKeyEqualityPredMBOT
{
public:
  bool operator()(const std::pair<Word, MBOTWord> k1,
                  const std::pair<Word, MBOTWord> k2) const {
    // Compare first non-terminal of each key.  Assumes that for Words
    // representing non-terminals only the first factor is relevant.
    {
      std::cout << "USING NON TERMINAL EQUALITY PRED" << std::endl;

      const Word & w1 = k1.first;
      std::cout << "First Word : " << w1 << std::endl;
      const Word & w2 = k2.first;
      std::cout << "Second Word : " << w2 << std::endl;
      const Factor * f1 = w1[0];
      std::cout << "First factor : " << *f1 << std::endl;
      const Factor * f2 = w2[0];
      std::cout << "Second factor : " << *f2 << std::endl;

      //note : factor comparison : if same return 0
      if (f1->Compare(*f2)) {
        std::cout << "Source Words are not the same !!!" << std::endl;
        return false;
      }
    }
    {
        // Compare second non-terminal of each key.
        const MBOTWord & w1 = k1.second;
        const MBOTWord & w2 = k2.second;

        //BEWARE : copy for removing const for comaprison
        MBOTWord w1copy = w1;
        MBOTWord w2copy = w2;

        if(w1copy != w2copy){
            //std::cout << "Target Words are not the same !!!" << std::endl;
            return false;
        }
    }
    return true;
  }
};

/** One node of the PhraseDictionaryMBOT structure
*/
class PhraseDictionaryNodeMBOT : public PhraseDictionaryNodeSCFG
{
public:

//TODO : WRITE MAPS CORRECTLY
typedef std::pair<Word, std::vector<Word> > NonTerminalMapKeyMBOT;
//typedef std::pair<Word,Word> TerminalMapKeyMBOT;

/*
#if defined(BOOST_VERSION) && (BOOST_VERSION >= 104200)
  typedef boost::unordered_map<Word,
          PhraseDictionaryNodeMBOT,
          TerminalHasherMBOT,
          TerminalEqualityPredMBOT> TerminalMapMBOT;

  typedef boost::unordered_map<NonTerminalMapKeyMBOT,
          PhraseDictionaryNodeMBOT,
          NonTerminalMapKeyHasherMBOT,
          NonTerminalMapKeyEqualityPredMBOT> NonTerminalMapMBOT;
    #else
    */
 typedef std::map<Word, PhraseDictionaryNodeMBOT> TerminalMapMBOT;
 typedef std::map<NonTerminalMapKeyMBOT, PhraseDictionaryNodeMBOT> NonTerminalMapMBOT;
//#endif

private:
  friend std::ostream& operator<<(std::ostream&, const PhraseDictionarySCFG&);
  friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryMBOT&);
  friend std::ostream& operator<<(std::ostream&, const TargetPhraseCollection&);

  // only these classes are allowed to instantiate this class
  //friend class PhraseDictionarySCFG;
  friend class PhraseDictionaryMBOT;
  friend class std::map<Word, PhraseDictionaryNodeMBOT>;
  //friend class std::map<Word, PhraseDictionaryNodeSCFG>;

protected:
  NonTerminalMapMBOT m_mbotNonTermMap;
  TerminalMapMBOT m_mbotSourceTermMap;
  TargetPhraseCollection *m_mbotTargetPhraseCollection;

  PhraseDictionaryNodeMBOT()
    : PhraseDictionaryNodeSCFG()
    , m_mbotTargetPhraseCollection(NULL)
  {
    //std::cout << "new PhraseDictionaryNodeMBOT()" << "At" << this << std::endl;
    //std::cout << "TPC" << GetTargetPhraseCollection() << std::endl;
  }

public:
  ~PhraseDictionaryNodeMBOT();

  bool IsLeafMBOT() const {
    return m_mbotSourceTermMap.empty() && m_mbotNonTermMap.empty();
  }

  bool IsLeaf() const {
   std::cout << "Is leaf for non mbot terminal map NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  const TargetPhraseCollection *GetTargetPhraseCollectionMBOT() const
  {
  //  std::cout << "PDMBOT : TARGET PHRASE COLLECTION 101 : " << &m_targetPhraseCollection << std::endl;
    return m_mbotTargetPhraseCollection;
  }

  //overwrite Method
  TargetPhraseCollection &GetOrCreateTargetPhraseCollectionMBOT(Word sourceLHS) {

    if (m_mbotTargetPhraseCollection == NULL)
    {

          m_mbotTargetPhraseCollection = new TargetPhraseCollection();
          //std::cout << "PDMBOT : CREATING TARGET PHRASE COLLECTION" << &m_mbotTargetPhraseCollection << std::endl;
    }
    else
    {
         //std::cout << "PDSCFG : GETTING TARGET PHRASE COLLECTION" << &m_mbotTargetPhraseCollection << std::endl;
    }

    return *m_mbotTargetPhraseCollection;
  }

  void Prune(size_t tableLimit)
  {
     std::cout << "Prune non mbot Phrase Dictionary NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  void PruneMBOT(size_t tableLimit);

  void Sort(size_t tableLimit)
  {
     std::cout << "Sort non mbot Phrase Dictionary NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  void SortMBOT(size_t tableLimit);

  PhraseDictionaryNodeSCFG *GetOrCreateChild(const Word &sourceTerm)
  {
    std::cout << "Get child NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  PhraseDictionaryNodeMBOT *GetOrCreateChildMBOT(const Word &sourceTerm);

  PhraseDictionaryNodeSCFG *GetOrCreateChild(const Word &sourceNonTerm, const Word &targetNonTerm)
  {
    std::cout << "Get child NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  PhraseDictionaryNodeMBOT *GetOrCreateChild(const Word &sourceNonTerm, const std::vector<Word> &targetNonTerm);

  const PhraseDictionaryNodeSCFG *GetChild(const Word &sourceTerm) const
  {
     std::cout << "Get child NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  const PhraseDictionaryNodeMBOT *GetChildMBOT(const Word &sourceTerm) const;

  const PhraseDictionaryNodeMBOT *GetChild(const Word &sourceNonTerm, const Word &targetNonTerm) const
  {
    std::cout << "Get child NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  const PhraseDictionaryNodeMBOT *GetChild(const Word &sourceNonTerm, const std::vector<Word> &targetNonTerm) const;

  const NonTerminalMap & GetNonTerminalMap() const
  {
     std::cout << "Get non mbot non term map NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  const NonTerminalMapMBOT & GetNonTerminalMapMBOT() const {
    return m_mbotNonTermMap;
  }

  void Clear()
  {
	  std::cout << "Clear non mbot Phrase dictionary node NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
	  ClearMBOT();
  }

  void ClearMBOT();

  TO_STRING();
};

std::ostream& operator<<(std::ostream&, const PhraseDictionaryNodeMBOT&);

}
