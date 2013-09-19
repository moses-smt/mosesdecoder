// $Id: TargetPhraseMBOT.h,v 1.1.1.1 2013/01/06 16:54:18 braunefe Exp $

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

/** represents an entry on the target side of a phrase table (scores, translation, alignment)
 */
class TargetPhraseMBOT : public TargetPhrase
{
    friend std::ostream& operator<<(std::ostream&, const TargetPhraseMBOT&);
    //friend ChartHypothesisMBOT;

protected:
    std::vector<Phrase> m_targetPhrases;
    const std::vector<const AlignmentInfoMBOT*> *m_alignments;
    std::vector<Word> m_targetLhs;

    //Fabienne Braune : Info that is usefull to have :
    Phrase const* m_sourcePhrase; //source phrase associated to target
    Word m_lhsSource; //source left-hand side
    bool m_matchesSource; //does a given source prhase match the input parse tree


public:

  TargetPhraseMBOT(Phrase sourcePhrase);
  TargetPhraseMBOT(std::string out_string,Phrase sourcePhrase);
  TargetPhraseMBOT(const Phrase &,Phrase sourcePhrase);
  TargetPhraseMBOT(TargetPhrase * tp);
  ~TargetPhraseMBOT();


  //does a target phrase match a syntax label on the source side of the rule ?
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

  //Set a whole vector of target lhs
  void SetTargetLHSMBOT(const std::vector<Word> &lhs) {
    m_targetLhs = lhs;
  }

  const std::vector<Word> &GetTargetLHSMBOT() const {
    return m_targetLhs;
  }

//! overloaded method from phrase
 const std::vector<Phrase> GetMBOTPhrases() const {
    return m_targetPhrases;
  }

  void SetMBOTPhrases(std::vector<Phrase> p)
  {
    m_targetPhrases = p;
  }

 size_t GetSize() const;

 //! get phrase at particular position in vector
 const Phrase * GetMBOTPhrase(size_t pos) const {
     //sanity check
    if(pos < m_targetPhrases.size() + 1)
    {return &m_targetPhrases[pos];}
    else
    {
        std::cout << "Error in TargetPhraseMBOT : out of vector bounds" << std::endl;
    }
  }

  void CreateFromStringNewFormat(FactorDirection direction
                                 , const std::vector<FactorType> &factorOrder
                                 , const std::string &phraseString
                                 , const std::string &factorDelimiter
                                 , std::vector<Word> &lhs);


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

//beware : inline removed and target words written in source file
  //! number of words : not implemented
  //! word at a particular position

//beware takes the first word of vector : used to initialize lm only...
  inline const Word &GetWord(size_t pos) const {
    return m_targetPhrases.front().GetWord(pos);
  }

  //fills the vector passed as argument with target phrases with words at positions pos
  inline size_t &GetWordVector(std::vector<std::vector<size_t> > pos, std::vector<Word> &wordVector) const {

        CHECK(pos.size() == GetMBOTPhrases().size());

        std::vector<Phrase> myPhrases = GetMBOTPhrases();
        std::vector<Phrase> :: const_iterator itr_phrase;
        std::vector<std::vector<size_t> > :: const_iterator itr_pos;
        int counter = 0;
        for(itr_pos = pos.begin(); itr_pos != pos.end(); itr_pos++)
        {
            //std::cout << "Counter : " << counter << std::endl;
            Phrase myPhrase = myPhrases[counter];

            std::vector<size_t> myPhPos = *itr_pos;
            std::vector<size_t> :: iterator itr_phpos;
            for(itr_phpos = myPhPos.begin();itr_phpos != myPhPos.end();itr_phpos++)
            {
                size_t currentPos = *itr_phpos;
                //std::cout << "Current Phrase " << myPhrase << std::endl;
                //std::cout << "Current Position "<< currentPos << std::endl;
                //std::cout << "Current word "<< myWord << std::endl;
                wordVector.push_back(myPhrase.GetWord(currentPos));
            }
            counter++;
        }

        //FB : hack : shouldn't return anything
        size_t vecSize = wordVector.size();
        return vecSize;

  }

  inline Word &GetWord(size_t pos) {
      std::cout << "Get single word NOT IMPLEMENTED in target phrase MBOT" << std::endl;
  }

  inline std::vector<Word> &GetWordVector (std::vector<size_t> pos){

        //new : check that there are as many phrases as positions
        CHECK(GetMBOTPhrases().size() == pos.size());

        //new : get word in target phrase vector
        //std::cout << "1. Getting word at position : "<< pos << std::endl;
        std::vector<Word> wordVector;
        Word myWord;

        std::vector<Phrase> :: const_iterator itr_phrase;
        std::vector<size_t> :: const_iterator itr_pos;
        for(itr_phrase = GetMBOTPhrases().begin(), itr_pos = pos.begin(); itr_phrase != GetMBOTPhrases().end(), itr_pos != pos.end(); itr_phrase++,itr_pos++)
        {
            Phrase myPhrase = *itr_phrase;
            size_t myPos = *itr_pos;
            //std::cout << "Current Phrase "<< myPhrase.GetSize() << std::endl;
            myWord = myPhrase.GetWord(myPos);
        }
        wordVector.push_back(myWord);
       //should return a vector of words
       return wordVector;
  }

  //! particular factor at a particular position : not implemented
  inline const Factor *GetFactor(size_t pos, FactorType factorType) const {
     return m_targetPhrases.front().GetFactor(pos,factorType);
  }

  inline void SetFactor(size_t pos, FactorType factorType, const Factor *factor) {
    std::cout << "Set Factor NOT IMPLEMENTED " << std::endl;
  }

  size_t GetNumTerminals() const;

  //! whether the 2D vector is a substring of this phrase
  bool Contains(const std::vector< std::vector<std::string> > &subPhraseVector
                , const std::vector<FactorType> &inputFactor) const;

  //! create an empty word at the end of the phrase
 //Beware : should only be used when processing unknown words
 Word &AddWord()
{
   Phrase firstPhrase = Phrase(1);
  //Phrase *firstPhrase = new Phrase(1);
  m_targetPhrases.push_back(firstPhrase);
  return m_targetPhrases.front().AddWord();
}

  //! create copy of input word at the end of the phrase
  void AddWord(const Word &newWord) {
      //Phrase *firstPhrase = new Phrase(1);
	  Phrase firstPhrase = Phrase(1);
      m_targetPhrases.push_back(firstPhrase);
      m_targetPhrases.front().AddWord(newWord);
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

    // Overloaded
    void SetAlignmentInfo(const StringPiece &alignString);
    //Set whole vector of alignments
    void SetAlignmentInfoVector(std::vector<std::set<std::pair<size_t,size_t> > > &alignmentInfoVector, std::set<size_t> &source, bool isSequence);

    //Fabienne Braune : alignment between terminals not implemented for now.
    void SetAlignTerm(const AlignmentInfo *alignTerm) {
    	std::cerr << "Alignments between terminals not yet implemented in l-MBOT system"<< std::endl;
    }

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
