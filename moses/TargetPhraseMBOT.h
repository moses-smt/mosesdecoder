//Fabienne Braune
//Special target phrase that can have several phrases on target side

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

#ifndef moses_TargetPhraseMBOT_h
#define moses_TargetPhraseMBOT_h

#include <vector>
#include "TypeDef.h"
#include "Phrase.h"
#include "ScoreComponentCollection.h"
#include "AlignmentInfoMBOT.h"
#include "AlignmentInfoCollectionMBOT.h"

#include <iostream>
#include <list>
#include <string>
#include "Word.h"
#include "WordsBitmap.h"
#include "TypeDef.h"
#include "Util.h"
#include "TargetPhrase.h"
#include "WordSequence.h"
#include "PhraseSequence.h"

#include "util/string_piece.hh"

#if HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_PROTOBUF
#include "rule.pb.h"
#endif


namespace Moses
{

class LMList;
class ScoreProducer;
class TranslationSystem;
class WordPenaltyProducer;

/** represents a target phrase having several discontiguous phrases (PhraseSequence)
 * and alignments (AlignmentSequence). Scores are inherited from base class.
 */
class TargetPhraseMBOT : public TargetPhrase
{
    friend std::ostream& operator<<(std::ostream&, const TargetPhraseMBOT&);
    //friend ChartHypothesisMBOT;

protected:
    //Fabienne Braune : This should either be const or a pointer. To fix.
    PhraseSequence m_targetPhrases;
    const std::vector<const AlignmentInfoMBOT*> *m_alignments;
    WordSequence m_targetLhs;

    //Fabienne Braune : source phrase corresponding to target
    bool m_matchesSource;
    Phrase* m_sourcePhrase;
    //Fabienne Braune : source left-hand side (parent nodes on source side)
    Word m_sourceLhs;

public:

  TargetPhraseMBOT(Phrase sourcePhrase);
  TargetPhraseMBOT(std::string out_string,Phrase sourcePhrase);
  TargetPhraseMBOT(const Phrase &,Phrase sourcePhrase);
  //TargetPhraseMBOT(TargetPhrase * tp);
  ~TargetPhraseMBOT();


  //Fabienne Braune : This allows to mark target phrases that match the input parse tree
  //During cube pruning (see class TranslationDimension in RuleCubeItem)
  virtual bool isMatchesSource() const
  {
	  return m_matchesSource;
  }

  virtual void setMatchesSource(bool matchesSource)
  {
   	m_matchesSource = matchesSource;
  }

  void SetTargetLHS(const Word &lhs) {
    std::cout << "Set target lhs with single Word NOT IMPLEMENTED for target phrase mbot" << std::endl;
  }

  const Word &GetTargetLHS() const {
    std::cout << "Get target lhs with single Word NOT IMPLEMENTED for target phrase mbot" << std::endl;
  }

  void SetSourceLHS(const Word &lhs) {
     m_sourceLhs = lhs;
   }

   const Word &GetSourceLHS() const {
     return m_sourceLhs;
   }

  void SetTargetLHSMBOT(const WordSequence &lhs) {
    m_targetLhs = lhs;
  }

  const WordSequence &GetTargetLHSMBOT() const {
    return m_targetLhs;
  }

 const PhraseSequence GetMBOTPhrases() const {
	std::cerr << "Trying to get : " << m_targetPhrases << std::endl;
    return m_targetPhrases;
 }

 // void SetMBOTPhrases(PhraseSequence p)
 // {
 //   m_targetPhrases = p;
 // }

 size_t GetSize() const;

 //! get phrase at particular position in vector
 const Phrase * GetMBOTPhrase(size_t pos) const {
     //sanity check
    if(pos < m_targetPhrases.GetSize() + 1)
    {return m_targetPhrases.GetPhrase(pos);}
    else
    {
        std::cerr<< "Error in TargetPhraseMBOT : out of vector bounds" << std::endl;
        abort();
    }
  }

  void CreateFromStringForSequence(FactorDirection direction
                                 , const std::vector<FactorType> &factorOrder
                                 , const StringPiece &phraseString
                                 , const std::string &factorDelimiter
                                 , WordSequence &lhs);


 void MergeFactors(const Phrase &copy);
  //! copy a single factor (specified by factorType) : not implemented
  void MergeFactors(const Phrase &copy, FactorType factorType);
  //! copy all factors specified in factorVec and none others : not inmplemented
  void MergeFactors(const Phrase &copy, const std::vector<FactorType>& factorVec);

  /** compare 2 phrases to ensure no factors are lost if the phrases are merged
  *	must run IsCompatible() to ensure incompatible factors aren't being overwritten
    : not implemented
  */
  bool IsCompatible(const Phrase &inputPhrase) const;
  bool IsCompatible(const Phrase &inputPhrase, FactorType factorType) const;
  bool IsCompatible(const Phrase &inputPhrase, const std::vector<FactorType>& factorVec) const;

  //TODO : remove this at some point
  //inline const Word &GetWord(size_t pos) const {
  //  return m_targetPhrases.front().GetWord(pos);
  //}

  //given a set of indices, fills the vector passed as arguments with words at those positions
  inline size_t &GetWordVector(std::vector<std::vector<size_t> > pos, WordSequence &wordVector) const {

        //std::cout << "MAKING MBOT PHRRASES "<< std::endl;

        CHECK(pos.size() == m_targetPhrases.GetSize());

        PhraseSequence :: const_iterator itr_phrase;
        std::vector<std::vector<size_t> > :: const_iterator itr_pos;
        int counter = 0;
        for(itr_pos = pos.begin(); itr_pos != pos.end(); itr_pos++)
        {
            std::vector<size_t> myPhPos = *itr_pos;
            std::vector<size_t> :: iterator itr_phpos;
            for(itr_phpos = myPhPos.begin();itr_phpos != myPhPos.end();itr_phpos++)
            {
                size_t currentPos = *itr_phpos;
                wordVector.Add(m_targetPhrases.GetPhrase(counter)->GetWord(currentPos));
            }
            counter++;
        }
        size_t vecSize = wordVector.GetSize();
        return vecSize;

  }

  inline std::vector<Word> &GetWordVector (std::vector<size_t> pos){

        //new : check that there are as many phrases as positions
        CHECK(m_targetPhrases.GetSize() == pos.size());

        //new : get word in target phrase vector
        //std::cout << "1. Getting word at position : "<< pos << std::endl;
        std::vector<Word> wordVector;
        Word myWord;

        PhraseSequence :: const_iterator itr_phrase;
        std::vector<size_t> :: const_iterator itr_pos;
        for(itr_phrase = m_targetPhrases.begin(), itr_pos = pos.begin(); itr_phrase != m_targetPhrases.end(), itr_pos != pos.end(); itr_phrase++,itr_pos++)
        {
            Phrase * myPhrase = *itr_phrase;
            size_t myPos = *itr_pos;
            //std::cout << "Current Phrase "<< myPhrase.GetSize() << std::endl;
            myWord = myPhrase->GetWord(myPos);
        }
        wordVector.push_back(myWord);
       //should return a vector of words
       return wordVector;
  }

  //just in case GetWord is called somewhere
  inline Word &GetWord(size_t pos) {
      std::cout << "Get single word NOT IMPLEMENTED in target phrase MBOT" << std::endl;
  }

  //! particular factor at a particular position : not implemented
  inline const Factor *GetFactor(size_t pos, FactorType factorType) const {
	  std::cout << "Factors NOT YET IMPLEMENTED FOR l-MBOT MODELS" << std::endl;
  }

  inline void SetFactor(size_t pos, FactorType factorType, const Factor *factor) {
    std::cout << "Set Factor NOT IMPLEMENTED " << std::endl;
  }

  size_t GetNumTerminals() const;

  //! whether the 2D vector is a substring of this phrase
  bool Contains(const std::vector< std::vector<std::string> > &subPhraseVector
                , const std::vector<FactorType> &inputFactor) const;

  //! create an empty word at the end of the phrase
  Word &AddWord()
 {
    Phrase firstPhrase = Phrase(1);
   m_targetPhrases.Add(&firstPhrase);
   return m_targetPhrases.GetPhrase(0)->AddWord();
 }

   //! create copy of input word at the end of the phrase
   void AddWord(const Word &newWord) {
 	  Phrase firstPhrase = Phrase(1);
 	 m_targetPhrases.Add(&firstPhrase);
 	    return m_targetPhrases.GetPhrase(0)->AddWord(newWord);
   }

  /** appends a phrase at the end of current phrase **/
  void Append(const Phrase &endPhrase);
  void PrependWord(const Word &newWord);

  void Clear() {
      std::cout << "Clear NOT IMPLEMENTED " << std::endl;
  }

  void CreateFromString(const std::vector<FactorType> &factorOrder, const StringPiece &phraseString, const StringPiece &factorDelimiter);

  void RemoveWord(size_t pos) {
    std::cout << "Remove word NOT IMPLEMENTED" << std::endl;
  }

  //! create new phrase class that is a substring of this phrase
  Phrase GetSubString(const WordsRange &wordsRange) const;

  //! return a string rep of the phrase. Each factor is separated by the factor delimiter as specified in StaticData class
  std::string GetStringRep(const std::vector<FactorType> factorsToPrint) const;

  TO_STRING();

  void InitializeMemPool();
  void FinalizeMemPool();

  int Compare(const TargetPhraseMBOT &other) const;


  /** transitive comparison between 2 phrases
   *		used to insert & find phrase in dictionary
   */
  bool operator< (const TargetPhraseMBOT &compare) const {
    return Compare(compare) < 0;
  }

  bool operator== (const TargetPhraseMBOT &compare) const {
    return Compare(compare) == 0;
  }

    //! overloaded
    void SetAlignmentInfo(const StringPiece &alignString);
    //void SetAlignmentInfo(const std::set<std::pair<size_t,size_t> > &alignmentInfo);

    void SetAlignmentInfoVector(std::vector<std::set<std::pair<size_t,size_t> > > &alignmentInfoVector, std::set<size_t> &source, bool isMBOT);

    //! overloaded but should not be used
    const AlignmentInfo &GetAlignmentInfo() const {
        std::cout << "Get single alignment info NOT IMPLEMENTED" << std::endl;
    }

    const std::vector<const AlignmentInfoMBOT*>* GetMBOTAlignments() const {
        return m_alignments;
    }

    void SetScoreChart(const ScoreProducer* translationScoreProducer,
                                    const Scores &scoreVector
                                 ,const std::vector<float> &weightT
                                 ,const LMList &languageModels
                                 ,const WordPenaltyProducer* wpProducer);
 };
}

#endif
