// $Id: TargetPhraseMBOT.cpp,v 1.1.1.1 2013/01/06 16:54:17 braunefe Exp $

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

#include "TargetPhrase.h"
#include "TargetPhraseMBOT.h"

#include "util/check.hh"
#include <algorithm>
#include <math.h>
#include <boost/lexical_cast.hpp>
#include "util/tokenize_piece.hh"

#include "PhraseDictionaryMemory.h"
#include "GenerationDictionary.h"
#include "LM/Base.h"
#include "StaticData.h"
#include "ScoreIndexManager.h"
#include "LMList.h"
#include "ScoreComponentCollection.h"
#include "Util.h"
#include "DummyScoreProducers.h"
#include "GapPenaltyProducer.h"
#include "AlignmentInfoCollection.h"

using namespace std;

namespace Moses
{

TargetPhraseMBOT::TargetPhraseMBOT(std::string out_string,Phrase sourcePhrase):TargetPhrase(out_string,sourcePhrase),
m_alignments(&AlignmentInfoCollectionMBOT::Instance().GetEmptyAlignmentInfoVector())
//initilize with for loop inside
{

   //std::cerr << "new TargetPhraseMBOT(1)" << std::endl;
    //new : FB : inserted for testing
   //std::cout << "Using constructor with string" << stdF::endl;
   //create vector
}

TargetPhraseMBOT::TargetPhraseMBOT(Phrase sourcePhrase):TargetPhrase(sourcePhrase),
m_alignments(&AlignmentInfoCollectionMBOT::Instance().GetEmptyAlignmentInfoVector())
{
     //std::cerr << "new TargetPhraseMBOT(2)" << std::endl;
     //new : FB : inserted for testing
    //std::cout << "Using constructor with nothing 1" << std::endl;
}

TargetPhraseMBOT::TargetPhraseMBOT(const Phrase &phrase,Phrase sourcePhrase):TargetPhrase(phrase, sourcePhrase),
m_alignments(&AlignmentInfoCollectionMBOT::Instance().GetEmptyAlignmentInfoVector())
{
    //std::cerr << "new TargetPhraseMBOT(3)" << this << std::endl;
     //new : FB : inserted for testing
     //std::cout << "Using constructor with nothing 2" << std::endl;
}

//copy constructor for target phrase
TargetPhraseMBOT::TargetPhraseMBOT(TargetPhrase * tp):TargetPhrase(tp),
		m_targetPhrases(static_cast<TargetPhraseMBOT*> (tp)->GetMBOTPhrases()),
		m_alignments(static_cast<TargetPhraseMBOT*> (tp)->GetMBOTAlignments()),
		m_targetLhs(static_cast<TargetPhraseMBOT*> (tp)->GetTargetLHSMBOT())
{
	//std::cout << "Breakdown from previous target phrase : " << tp->GetScoreBreakdown() << std::endl;
	//std::cout << "Copy constructor for target phrase MBOT : " << m_scoreBreakdown << std::endl;
	//std::cerr << "Constructing new Target Phrase " << static_cast<TargetPhraseMBOT*> (tp)->GetTargetLHSMBOT().size() <<  std::endl;
}


TargetPhraseMBOT::~TargetPhraseMBOT()
{

	//RemoveAllInCollection(m_alignments);
	 /*std::vector<AlignmentInfo*> ::iterator itr_info;
	    for(itr_info = m_alignments.begin(); itr_info != m_alignments.end(); itr_info++)
	    {
	        AlignmentInfo thisInfo = **itr_info;
	        //std::cout << thisInfo << std::endl;
	        delete *itr_info;
	    }*/

}

//overloaded
//create
void TargetPhraseMBOT::CreateFromStringNewFormat(FactorDirection direction
                                       , const std::vector<FactorType> &factorOrder
                                       , const std::string &phraseString
                                       , const std::string & /*factorDelimiter */
                                       , std::vector<Word> &lhs)
{
  // parse
  //f+n : separate target phrases at ||
  vector<string> targetPhraseFields;
  vector<string>::iterator itr_tagetPhraseFields;
  TokenizeMultiCharSeparator(targetPhraseFields, phraseString, "||");

   //new : inserted for testing
  //std::cout << "Phrase string is "<< phraseString << std::endl;

  for(itr_tagetPhraseFields = targetPhraseFields.begin(); itr_tagetPhraseFields != targetPhraseFields.end(); itr_tagetPhraseFields++)
  {

        //new 1: initialize vector of words for each field
        //std::vector<Word*> wordsInPhrase;
        string field = *itr_tagetPhraseFields;
        //new .: FB : inserted for testing
        //std::cout << "Target field : " << field << std::endl;
        vector<string> annotatedWordVector;
        //new .: FB : inserted for testing
        Tokenize(annotatedWordVector, field);

        //std::cout << "Size of annotated field vector : " << annotatedWordVector.size() << std::endl;
        //new : FB : beware : last field of annotated Words Vector contains LHS INFO !!!
        Phrase myPhrase = Phrase(annotatedWordVector.size() -1);

        // KOMMA|none ART|Def.Z NN|Neut.NotGen.Sg VVFIN|none
        //		to
        // "KOMMA|none" "ART|Def.Z" "NN|Neut.NotGen.Sg" "VVFIN|none"

        //new : FB : BEWARE : modified annotatedWordVector.size() -1 to annotatedWordVector.size()
        //new : FB : BEWARE : annotatedWordVector must leave one field which contains LHS INFO !!!
        for (size_t phrasePos = 0 ; phrasePos < annotatedWordVector.size() -1; phrasePos++) {
        string &annotatedWord = annotatedWordVector[phrasePos];

        //new : inserted for testing
        //std::cout << "annotated Word before field " << annotatedWord << std::endl;
        //std::cout << "first substring of word " << annotatedWord.substr(0, 1) << std::endl;
        //std::cout << "second substring of word " << annotatedWord.substr(annotatedWord.size()-1, 1) << std::endl;

        bool isNonTerminal;
        if (annotatedWord.substr(0, 1) == "[" && annotatedWord.substr(annotatedWord.size()-1, 1) == "]") {
          // non-term
          isNonTerminal = true;

          size_t nextPos = annotatedWord.find("[", 1);
          //f+n: inserted for testing
          //std::cout << "nextPos " << nextPos << std::endl;
          //std::cout << "npos " << string::npos << std::endl;

          //f+n: start of our code
          if ( nextPos != string::npos) {
            if (direction == Input)
            {
                annotatedWord = annotatedWord.substr(1, nextPos - 2);
                //std::cout << "Annotated substring last: " << annotatedWord << std::endl;
            }
            else
            {
                annotatedWord = annotatedWord.substr(nextPos + 1, annotatedWord.size() - nextPos - 2);
                //std::cout << "Annotated substring last: " << annotatedWord << std::endl;
          }
        }
      }
      else {
          isNonTerminal = false;
    }
        Word &myWord = myPhrase.AddWord();
        myWord.CreateFromString(direction, factorOrder, annotatedWord, isNonTerminal);
    }
    m_targetPhrases.push_back(myPhrase);

    // lhs
    string &annotatedWord = annotatedWordVector.back();
    //std::cout << "ANNOTATED WORD : " << annotatedWord << std::endl;
    CHECK(annotatedWord.substr(0, 1) == "[" && annotatedWord.substr(annotatedWord.size() -1 , 1) == "]");
    annotatedWord = annotatedWord.substr(1, annotatedWord.size() - 2);

    Word myLhs;
    myLhs.CreateFromString(direction, factorOrder, annotatedWord, true);
    CHECK(myLhs.IsNonTerminal());
    lhs.push_back(myLhs);

    }
}

//Just in case those are called by mistake somewhere
void TargetPhraseMBOT::MergeFactors(const Phrase &copy)
{
    cout << "Merge Factors not implemented for targetPhrase MBOT" << endl;
}

void TargetPhraseMBOT::MergeFactors(const Phrase &copy, FactorType factorType)
{
     cout << "Merge Factors not implemented for targetPhrase MBOT" << endl;
}

void TargetPhraseMBOT::MergeFactors(const Phrase &copy, const std::vector<FactorType>& factorVec)
{
     cout << "Merge Factors not implemented for targetPhrase MBOT" << endl;
}


Phrase TargetPhraseMBOT::GetSubString(const WordsRange &wordsRange) const
{
     cout << "Get Substring not implemented for targetPhrase MBOT" << endl;
}

void TargetPhraseMBOT::Append(const Phrase &endPhrase)
{
     cout << "Append not implemented for targetPhrase MBOT" << endl;
}

void TargetPhraseMBOT::PrependWord(const Word &newWord)
{
     cout << "Prepend word not implemented for targetPhrase MBOT" << endl;
}

void TargetPhraseMBOT::CreateFromString(const std::vector<FactorType> &factorOrder, const StringPiece &phraseString, const StringPiece &factorDelimiter)
{
     cout << "Create from string not implemented for targetPhrase MBOT" << endl;
}

//Compare :
//1. compare sizes of phrase vector
//2. if sizes are the same : compare each phrase return first case where phrases are different
int TargetPhraseMBOT::Compare(const TargetPhraseMBOT &other) const
{
    size_t thisSize = GetMBOTPhrases().size();
    size_t compareSize = other.GetMBOTPhrases().size();

    if (thisSize != compareSize) {
    return (thisSize < compareSize) ? -1 : 1;
  }

    for(size_t pos = 0; pos < thisSize; pos++)
    {
        const Phrase * otherPhrase = other.GetMBOTPhrase(pos);
        int ret = Phrase::Compare(*otherPhrase);

        if(ret!=0)
        return ret;
    }

    return 0;
}

std::string TargetPhraseMBOT::GetStringRep(const vector<FactorType> factorsToPrint) const
{
     cout << "GetStringRep not implemented for targetPhrase MBOT" << endl;
     cout << "GetStringRep returns empty stream" << endl;
     stringstream strme;
     return strme.str();
}

bool TargetPhraseMBOT::Contains(const vector< vector<string> > &subPhraseVector
                      , const vector<FactorType> &inputFactor) const
{
    bool empty_bool;
    cout << "Contains not implemented for targetPhrase MBOT" << endl;

    return empty_bool;
}

bool TargetPhraseMBOT::IsCompatible(const Phrase &inputPhrase) const
{
    bool empty_bool;
    cout << "Is compatible not implemented for targetPhrase MBOT" << endl;

    return empty_bool;
}

bool TargetPhraseMBOT::IsCompatible(const Phrase &inputPhrase, FactorType factorType) const
{
     bool empty_bool;
     cout << "IsCompatible not implemented for targetPhrase MBOT" << endl;

     return empty_bool;
}

bool TargetPhraseMBOT::IsCompatible(const Phrase &inputPhrase, const std::vector<FactorType>& factorVec) const
{
     bool empty_bool;
     cout << "IsCompatible not implemented for targetPhrase MBOT" << endl;

     return empty_bool;
}

//Get size of target phrase
//BEWARE : may be called for span width
//Here we return the sum of the sizes of all (possibly discontinuous) phrase in target prhase mbot
size_t TargetPhraseMBOT::GetSize() const {
    std::vector<Phrase> :: const_iterator itr_targetPhrases;
    size_t sumOfsizes = 0;
    for(itr_targetPhrases = m_targetPhrases.begin();itr_targetPhrases != m_targetPhrases.end();itr_targetPhrases++)
    {
        Phrase currentPhrase = *itr_targetPhrases;
        sumOfsizes += currentPhrase.GetSize();
    }
    return sumOfsizes;
  }

//Get amount of terminals for computing score
//BEWARE : called inSetScoreChart of Target Phrase
//Here we return the sum of the terminals in all (possibly discontinuous target phrases)
size_t TargetPhraseMBOT::GetNumTerminals() const
{
    std::vector<Phrase> :: const_iterator itr_targetPhrases;
    size_t sumOfterms = 0;
    for(itr_targetPhrases = m_targetPhrases.begin();itr_targetPhrases != m_targetPhrases.end();itr_targetPhrases++)
    {

        Phrase currentPhrase = *itr_targetPhrases;
        sumOfterms += currentPhrase.GetNumTerminals();
    }

  return sumOfterms;
}

void TargetPhraseMBOT::InitializeMemPool()
{
      cout << "InitilizeMemPool not implemented for targetPhrase MBOT" << endl;
}

void TargetPhraseMBOT::FinalizeMemPool()
{
      cout << "Finalize MemPool not implemented for targetPhrase MBOT" << endl;
}


namespace {
void MosesShouldUseExceptions(bool value) {
  if (!value) {
    std::cerr << "Could not parse alignment info" << std::endl;
    abort();
  }
}
} // namespace

//For setting l-MBOT rule alignments, we have to also tokenize at || and create a vector of alignment infos to represent the alignment sequences.
//More explanations in AlignmentInfoMBOT.h
//TODO : handle alignments between terminals
void TargetPhraseMBOT::SetAlignmentInfo(const StringPiece &alignString)
{
    string alignStringConv;
    alignStringConv = alignString.as_string();

    //tokenize alignment string at || for discontiguous target phrases
    vector<string> alignStringFields;
    vector<string> :: iterator itr_align_fields;

    TokenizeMultiCharSeparator(alignStringFields, alignStringConv, "||");

    //collect all source indices
    std::set<size_t> sourceIndices;
    //indicate if alignment is discontiguous
    bool isSequence = 0;
    if(alignStringFields.size() > 1){isSequence = 1;}
    for(itr_align_fields = alignStringFields.begin(); itr_align_fields != alignStringFields.end(); itr_align_fields++)
    {
        StringPiece alignField = *itr_align_fields;

        for (util::TokenIter<util::AnyCharacter, true> token(alignField, util::AnyCharacter(" \t")); token; ++token) {
        util::TokenIter<util::AnyCharacter, false> dash(*token, util::AnyCharacter("-"));
        MosesShouldUseExceptions(dash);
        size_t sourcePos = boost::lexical_cast<size_t>(*dash++);
        MosesShouldUseExceptions(dash);
        sourceIndices.insert(sourcePos);
        }
    }
    //Use vector instead of single alignment info
    vector<set<pair<size_t,size_t> > > alignmentInfoVector;

    for(itr_align_fields = alignStringFields.begin(); itr_align_fields != alignStringFields.end(); itr_align_fields++)
    {
    	 StringPiece alignField = *itr_align_fields;

        set<pair<size_t,size_t> > alignmentInfo;

        for (util::TokenIter<util::AnyCharacter, true> token(alignField, util::AnyCharacter(" \t")); token; ++token) {
        util::TokenIter<util::AnyCharacter, false> dash(*token, util::AnyCharacter("-"));
        MosesShouldUseExceptions(dash);
        size_t sourcePos = boost::lexical_cast<size_t>(*dash++);
        MosesShouldUseExceptions(dash);
        size_t targetPos = boost::lexical_cast<size_t>(*dash++);
        MosesShouldUseExceptions(!dash);
        alignmentInfo.insert(pair<size_t,size_t>(sourcePos, targetPos));
        }
        alignmentInfoVector.push_back(alignmentInfo);
    }
    SetAlignmentInfoVector(alignmentInfoVector,sourceIndices,isSequence);
}

void TargetPhraseMBOT::SetAlignmentInfoVector(std::vector<std::set<std::pair<size_t,size_t> > > &alignmentInfoVector, std::set<size_t> &source, bool isMBOT)
{
	m_alignments = AlignmentInfoCollectionMBOT::Instance().AddVector(alignmentInfoVector,source,isMBOT);
}

void TargetPhraseMBOT::SetScoreChart(const ScoreProducer* translationScoreProducer,
                                 const Scores &scoreVector
                                 ,const vector<float> &weightT
                                 ,const LMList &languageModels
                                 ,const WordPenaltyProducer* wpProducer)
{
  CHECK(weightT.size() == scoreVector.size());

  // calc average score if non-best
  m_transScore = std::inner_product(scoreVector.begin(), scoreVector.end(), weightT.begin(), 0.0f);
  m_scoreBreakdown.PlusEquals(translationScoreProducer, scoreVector);

  // Replicated from TranslationOptions.cpp
  float totalNgramScore  = 0;
  float totalFullScore   = 0;
  float totalOOVScore    = 0;

  LMList::const_iterator lmIter;
  for (lmIter = languageModels.begin(); lmIter != languageModels.end(); ++lmIter) {
    const LanguageModel &lm = **lmIter;

    if (lm.Useable(*this)) {
      // contains factors used by this LM
      const float weightLM = lm.GetWeight();
      const float oovWeightLM = lm.GetOOVWeight();
      float fullScore, nGramScore;
      size_t oovCount;

      lm.CalcScoreMBOT(*this, fullScore, nGramScore, oovCount);
      fullScore = UntransformLMScore(fullScore);
      nGramScore = UntransformLMScore(nGramScore);

      //std::cout << "nGram Score : " << totalNgramScore << std::endl;

      if (StaticData::Instance().GetLMEnableOOVFeature()) {
        vector<float> scores(2);
        scores[0] = nGramScore;
        scores[1] = oovCount;
        m_scoreBreakdown.Assign(&lm, scores);
        totalOOVScore += oovCount * oovWeightLM;
      } else {
        m_scoreBreakdown.Assign(&lm, nGramScore);
      }

      // total LM score so far
      totalNgramScore  += nGramScore * weightLM;
      //std::cout << "Total n-gram score : " << totalNgramScore << std::endl;
      totalFullScore   += fullScore * weightLM;
      //std::cout << "Full score : " << totalFullScore << std::endl;
    }
  }

  size_t wordCount = GetNumTerminals();
  m_scoreBreakdown.Assign(wpProducer, - (float) wordCount * 0.434294482); // TODO log -> ln ??

  //gap penalty
  //check if gap penalty is enabled otherwise take log(1)

  const GapPenaltyProducer *gpp = StaticData::Instance().GetGapPenaltyProducer();

  if(gpp != NULL)
  {
	  int nbrMBOT = GetMBOTPhrases().size();
	  //Discount mbot gap
	  float gapPenalty = 1/(pow(100,nbrMBOT-1));
  	  m_scoreBreakdown.Assign(gpp, log(gapPenalty)); // TODO log -> ln ??
  }

  m_fullScore = m_scoreBreakdown.GetWeightedScore() - totalNgramScore + totalFullScore + totalOOVScore;

 // std::cout << "SCORE COMPONENENT BREAKDOWN FOR THIS TARGET PHRASE : " << m_scoreBreakdown << std::endl;
  //std::cout << "Set future score : " << m_fullScore << std::endl;
  // std::cout << "TP : " << *this << std::endl;
}

//void TargetPhraseMBOT::SetAlignmentInfo(const std::set<std::pair<size_t,size_t> > &alignmentInfo)
//{
    //new : when setting alignment pushback one alignment into vector
    //std::cout << "Setting alignment info" << std::endl;
//    AlignmentInfo createdAlignInfo = *AlignmentInfoCollection::Instance().Add(alignmentInfo);
//    std::cout << "Created alignment : "<< createdAlignInfo << std::endl;
//    m_alignments.push_back(&createdAlignInfo);
//}


// friend
ostream& operator<<(ostream& out, const TargetPhraseMBOT& targetPhrase)
{
  out << std::endl;
  out << "-------------------------------------" << std::endl;
  out << "THIS : IS AN MBOT TARGET PRHASE " << std::endl;
  out << "-------------------------------------" << std::endl;

  const std::vector<Phrase> myPhrases = targetPhrase.GetMBOTPhrases();
  std::vector<Phrase> :: const_iterator itr_vector_phrases;

  out << "Target Phrases : ";
  int phraseCounter = 1;

  for(itr_vector_phrases = myPhrases.begin(); itr_vector_phrases != myPhrases.end(); itr_vector_phrases++)
    {
        Phrase currentPhrase = *itr_vector_phrases;
        out << currentPhrase << "(" << phraseCounter++ << ") ";
    }
    out << "\t" << std::endl;

  //FB : alignments destroyed with target phrase
  std::vector<const AlignmentInfoMBOT*> :: const_iterator itr_vector_align;
  const std::vector<const AlignmentInfoMBOT*> *mbotAlignments = targetPhrase.GetMBOTAlignments();

  out << "Alignment Info : ";
  int alignmentCounter = 0;
  for(itr_vector_align = mbotAlignments->begin(); itr_vector_align != mbotAlignments->end(); itr_vector_align++)
    {
        //out << "In LOOP" << endl;
        const AlignmentInfoMBOT* infoToPrint = *itr_vector_align;
        if(infoToPrint != NULL)
        {
            out << *infoToPrint << "(" << ++alignmentCounter << ") ";
            //out << "Alignment map : ";
            //std::vector<size_t> :: const_iterator itr_map;
            //for(itr_map = infoToPrint->GetNonTermIndexMap()->begin(); itr_map != infoToPrint->GetNonTermIndexMap()->end(); itr_map++)
            //{ out << *itr_map << std::endl; }
        }
        else{out << "No alignment";}
    }
    out << std::endl;

  out << "Translation Score " << targetPhrase.GetTranslationScore() << endl;
  out << "Full Score " << targetPhrase.GetFutureScore() << endl;
  out << "Feature Vector " << targetPhrase.GetScoreBreakdown() << endl;
  out << "Source Phrase : " << *(targetPhrase.GetSourcePhrase()) << endl;
  out << "Source Left-hand-side : " << targetPhrase.GetSourceLHS() << endl;
  out << "Target Left-hand-side : ";

  std::vector<Word> :: const_iterator itr_words;

  int counter = 1;
  for(itr_words = targetPhrase.GetTargetLHSMBOT().begin(); itr_words != targetPhrase.GetTargetLHSMBOT().end(); itr_words++)
  {
        out << *itr_words << "(" << counter++ << ") ";
  }
  out << endl;
  out << "-------------------------------------" << std::endl;

  return out;
}

}
