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

#include <iostream>
#include <limits>
#include <assert.h>
#include "TranslationOption.h"
#include "TranslationOptionCollection.h"
#include "Hypothesis.h"
#include "Util.h"
#include "Arc.h"
#include "SquareMatrix.h"
#include "LexicalReordering.h"
#include "StaticData.h"
#include "Input.h"
#include "LMList.h"
#include "md5.h"

using namespace std;

unsigned int Hypothesis::s_numNodes = 0;
unsigned int Hypothesis::s_HypothesesCreated = 0;


Hypothesis::Hypothesis(InputType const& source)
	: LatticeEdge(Output)
	, m_sourceCompleted(source.GetSize())
	, m_sourceInput(source)
	, m_currSourceWordsRange(NOT_FOUND, NOT_FOUND)
	, m_currTargetWordsRange(NOT_FOUND, NOT_FOUND)
	, m_wordDeleted(false)
#ifdef N_BEST
	, m_arcList(0)
#endif
	, m_id(s_HypothesesCreated++)
{	// used for initial seeding of trans process	
	// initialize scores
	ResetScore();	
}

Hypothesis::Hypothesis(const Hypothesis &copy)
	: LatticeEdge							(Output, copy.m_prevHypo)
	, m_sourceCompleted				(copy.m_sourceCompleted )
	, m_sourceInput           (copy.m_sourceInput)
	, m_currSourceWordsRange	(copy.m_currSourceWordsRange)
	, m_currTargetWordsRange	(copy.m_currTargetWordsRange)
	, m_wordDeleted						(false)
#ifdef N_BEST
	, m_arcList(0)
#endif
	, m_id										(s_HypothesesCreated++)
{
	m_targetPhrase.AddWords( copy.m_targetPhrase );

	// initialize scores
	SetScore(copy.GetScore());
}

/***
 * continue prevHypo by appending the phrases in transOpt
 */
Hypothesis::Hypothesis(const Hypothesis &prevHypo, const TranslationOption &transOpt)
	: LatticeEdge							(Output, &prevHypo)

	, m_sourceCompleted				(prevHypo.m_sourceCompleted )
	, m_sourceInput					(prevHypo.m_sourceInput)
	, m_currSourceWordsRange	(transOpt.GetSourceWordsRange())
	, m_currTargetWordsRange		( prevHypo.m_currTargetWordsRange.GetEndPos() + 1
														 ,prevHypo.m_currTargetWordsRange.GetEndPos() + transOpt.GetPhrase().GetSize())
	, m_wordDeleted(false)
#ifdef N_BEST
	, m_arcList(0)
#endif
	, m_id(s_HypothesesCreated++)
{
	const Phrase &possPhrase				= transOpt.GetTargetPhrase();
	
	m_sourceCompleted.SetValue(m_currSourceWordsRange.GetStartPos(), m_currSourceWordsRange.GetEndPos(), true);
	// add new words from poss trans
	//m_phrase.AddWords(prev.m_phrase);
	m_targetPhrase.AddWords(possPhrase);

	// scores
	SetScore(prevHypo.GetScore());
	m_score[ScoreType::PhraseTrans]					+= transOpt.GetTranslationScore();
	m_score[ScoreType::FutureScoreEnum]			+= transOpt.GetFutureScore();
	m_score[ScoreType::LanguageModelScore]	+= transOpt.GetNgramScore();
	m_score[ScoreType::Generation]					+= transOpt.GetGenerationScore();
	
  m_wordDeleted = transOpt.IsDeletionOption();

#ifdef N_BEST
	// language model score (ngram)
	m_lmScoreComponent.Combine(transOpt.GetNgramComponent());

	// translation score
	m_transScoreComponent.Combine(transOpt.GetTransScoreComponent());
	
	// generation score
	m_generationScoreComponent.Combine(transOpt.GetGenerationScoreComponent());
		
#endif
}

Hypothesis::~Hypothesis()
{
#ifdef N_BEST
	if (m_arcList) {
		RemoveAllInColl< vector<Arc*>::iterator >(*m_arcList);
		delete m_arcList;
	}
#endif
}

#ifdef N_BEST
void Hypothesis::AddArc(Hypothesis &loserHypo)
{
	if (!m_arcList) {
		if (loserHypo.m_arcList)  // we don't have an arcList, but loser does
		{
			this->m_arcList = loserHypo.m_arcList;  // take ownership, we'll delete
			loserHypo.m_arcList = 0;                // prevent a double deletion
		}
		else
			{ this->m_arcList = new std::vector<Arc*>(); }
	} else {
		if (loserHypo.m_arcList) {  // both have an arc list: merge. delete loser
			size_t my_size = m_arcList->size();
			size_t add_size = loserHypo.m_arcList->size();
			this->m_arcList->resize(my_size + add_size, 0);
			std::memcpy(&(*m_arcList)[0] + my_size, &(*m_arcList)[0], add_size * sizeof(Arc *));
			delete loserHypo.m_arcList;
			loserHypo.m_arcList = 0;
		} else { // loserHypo doesn't have any arcs
		  // DO NOTHING
		}
	}
	Arc *arc = new Arc(loserHypo.m_score
                    , loserHypo.GetTranslationScoreComponent()
                    , loserHypo.GetLMScoreComponent()
                    , loserHypo.GetGenerationScoreComponent()
                    , loserHypo.GetPhrase()
                    , loserHypo.GetPrevHypo());
	m_arcList->push_back(arc);
}
#endif

/***
 * return the subclass of Hypothesis most appropriate to the given translation option
 */
Hypothesis* Hypothesis::CreateNext(const TranslationOption &transOpt) const
{
	return Create(*this, transOpt);
}

/***
 * return the subclass of Hypothesis most appropriate to the given translation option
 */
Hypothesis* Hypothesis::Create(const Hypothesis &prevHypo, const TranslationOption &transOpt)
{
	return new Hypothesis(prevHypo, transOpt);
}
/***
 * return the subclass of Hypothesis most appropriate to the given target phrase
 */

Hypothesis* Hypothesis::Create(InputType const& m_source)
{
	return new Hypothesis(m_source);
}

/***
 * if any factors aren't set in our target phrase but are present in transOpt, copy them over
 * (unless the factors that we do have fail to match the corresponding ones in transOpt,
 *  in which case presumably there's a programmer's error)
 * 
 * return NULL if we aren't compatible with the given option
 */
Hypothesis* Hypothesis::MergeNext(const TranslationOption &transOpt) const
{
	// check each word is compatible and merge 1-by-1
	const Phrase &possPhrase = transOpt.GetTargetPhrase();
	if (! IsCompatible(possPhrase))
	{
		return NULL;
	}

	// ok, merge
	Hypothesis* clone = new Hypothesis(*this);

	int currWord = 0;
	size_t len = GetSize();
	for (size_t currPos = len - m_currTargetWordsRange.GetWordsCount() ; currPos < len ; currPos++)
	{
		const FactorArray &sourceWord	= possPhrase.GetFactorArray(currWord);
		FactorArray &targetWord = clone->GetCurrFactorArray(currPos - m_currTargetWordsRange.GetStartPos());
		Word::Merge(targetWord, sourceWord);
		
		currWord++;
	}

#ifdef N_BEST
	const ScoreComponentCollection &transOptComponent = transOpt.GetTransScoreComponent();
	clone->m_transScoreComponent.Combine(transOptComponent);
#endif

	return clone;

}

void Hypothesis::MergeFactors(vector< const Word* > mergeWords, const GenerationDictionary &generationDictionary, float generationScore, float weight)
{
	assert (mergeWords.size() == m_currTargetWordsRange.GetWordsCount());

	const size_t startPos = GetSize() - m_currTargetWordsRange.GetWordsCount();

	int mergeWordPos = 0;
	for (size_t currPos = startPos ; currPos < startPos + mergeWords.size() ; currPos++)
	{
		const Word &mergeWord = *mergeWords[mergeWordPos];
		FactorArray &origWord	= GetCurrFactorArray(currPos - m_currTargetWordsRange.GetStartPos());

		for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
		{
			FactorType factorType = static_cast<FactorType>(currFactor);
			const Factor *factor = mergeWord.GetFactor(factorType);
			if (factor != NULL)
			{
				origWord[factorType] = factor;
			}
		}

		mergeWordPos++;
	}

	// score
	m_score[ScoreType::Generation] += generationScore * weight;
#ifdef N_BEST
	m_generationScoreComponent[(size_t) &generationDictionary] += generationScore;
#endif
}

bool Hypothesis::IsCompatible(const Phrase &phrase) const
{
	// make sure factors don't contradict each other
	// similar to phrase comparison

	if (m_currTargetWordsRange.GetWordsCount() != phrase.GetSize())
	{
		return false;
	}
	size_t hypoSize = GetSize();

	size_t transOptPos = 0;
	for (size_t hypoPos = hypoSize - m_currTargetWordsRange.GetWordsCount() ; hypoPos < hypoSize ; hypoPos++)
	{
		for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
		{
			FactorType factorType = static_cast<FactorType>(currFactor);
			const Factor *thisFactor 		= GetFactor(hypoPos, factorType)
									,*compareFactor	= phrase.GetFactor(transOptPos, factorType);
			if (thisFactor != NULL && compareFactor != NULL && thisFactor != compareFactor)
				return false;
		}
		transOptPos++;
	}
	return true;
}

void Hypothesis::GenerateNGramCompareKey(size_t contextSize)
{
  struct MD5Context md5c;

  MD5Init(&md5c);
  size_t thisSize = this->GetSize();
  size_t effectiveContextSize = std::min(thisSize, contextSize);
  int start = thisSize - effectiveContextSize;

  if (m_currTargetWordsRange.GetWordsCount() > 0) // initial hypothesis check
	{
	  const Hypothesis *curHyp = this;
	  int curStart = 0;
	  while (start < (curStart = curHyp->m_currTargetWordsRange.GetStartPos())) {
	    for (int col = curHyp->m_currTargetWordsRange.GetEndPos(); col >= curStart; col--) {
	      MD5Update(&md5c,
	        (unsigned char*)curHyp->GetCurrFactorArray(col - curStart),
	        sizeof(FactorArray));
				curHyp = curHyp->m_prevHypo;
	    }
	  }
	  for (int col = curHyp->m_currTargetWordsRange.GetEndPos(); col >= (int)start; col--) {
	    MD5Update(&md5c,
	      (unsigned char*)curHyp->GetCurrFactorArray(col - curStart),
	      sizeof(FactorArray));
	  }
	}
  MD5Final(m_compSignature, &md5c);
}

int Hypothesis::NGramCompare(const Hypothesis &compare, size_t nGramSize[NUM_FACTORS]) const
{ // -1 = this < compare
	// +1 = this > compare
	// 0	= this ==compare

	const size_t thisSize			= GetSize();
	const size_t compareSize	= compare.GetSize();

	for (size_t currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
	{
		const size_t minSize		= std::min(nGramSize[currFactor], thisSize)
					, minCompareSize	= std::min(nGramSize[currFactor], compareSize);
		if ( minSize != minCompareSize )
		{ // quick decision
			return (minSize < minCompareSize) ? -1 : 1;
		}

		for (size_t currNGram = 1 ; currNGram <= minSize ; currNGram++)
		{
			FactorType factorType = static_cast<FactorType>(currFactor);
			const Factor *thisFactor 		= GetFactor(thisSize - currNGram, factorType)
				,*compareFactor	= compare.GetFactor(compareSize - currNGram, factorType);
			// just use address of factor
			if (thisFactor < compareFactor)
				return -1;
			if (thisFactor > compareFactor)
				return 1;
		}
	}
	// identical
	return 0;
}
/**
 * Calculates the overall language model score by combining the scores
 * of language models generated for each of the factors.  Because the factors
 * represent a variety of tag sets, and because factors with smaller tag sets 
 * (such as POS instead of words) allow us to calculate richer statistics, we
 * allow a different length of n-gram to be specified for each factor.
 * /param lmListInitial todo - describe this parameter 
 * /param lmListEnd todo - describe this parameter
 */
void Hypothesis::CalcLMScore(const LMList &lmListInitial, const LMList	&lmListEnd)
{
	const size_t startPos	= m_currTargetWordsRange.GetStartPos();
	LMList::const_iterator iterLM;

	// for LM which are not in PossTran
	// must go through each trigram in current phrase
	for (iterLM = lmListEnd.begin() ; iterLM != lmListEnd.end() ; ++iterLM)
	{
		const LanguageModel &languageModel = **iterLM;
		FactorType factorType	= languageModel.GetFactorType();
		size_t nGramOrder			= languageModel.GetNGramOrder();
		float lmScore;

		// 1st n-gram
		vector<const Factor*> contextFactor(nGramOrder);
		size_t index = 0;
		for (int currPos = (int) startPos - (int) nGramOrder + 1 ; currPos <= (int) startPos ; currPos++)
		{
			if (currPos >= 0)
				contextFactor[index++] = GetFactor(currPos, factorType);
			else			
				contextFactor[index++] = languageModel.GetSentenceStart();
		}		
		lmScore	= languageModel.GetValue(contextFactor);
		//cout<<"context factor: "<<languageModel.GetValue(contextFactor)<<endl;

		// main loop
		for (size_t currPos = startPos + 1 ; currPos <= m_currTargetWordsRange.GetEndPos() ; currPos++)
		{
			// shift all args down 1 place
			for (size_t i = 0 ; i < nGramOrder - 1 ; i++)
				contextFactor[i] = contextFactor[i + 1];

			// add last factor
			contextFactor.back() = GetFactor(currPos, factorType);

			lmScore	+= languageModel.GetValue(contextFactor);
			//cout<<"context factor: "<<languageModel.GetValue(contextFactor)<<endl;
		
		}

		// end of sentence
		if (m_sourceCompleted.IsComplete())
		{
			// shift all args down 1 place
			for (size_t i = 0 ; i < nGramOrder - 1 ; i++)
				contextFactor[i] = contextFactor[i + 1];

			contextFactor.back() = languageModel.GetSentenceEnd();
			lmScore	+= languageModel.GetValue(contextFactor);
		}
		m_score[ScoreType::LanguageModelScore] += lmScore * languageModel.GetWeight();
#ifdef N_BEST
		size_t lmId = languageModel.GetId();
		m_lmScoreComponent[lmId] += lmScore;
#endif
	}

	// for LM which are in possTran
	// already have LM scores from previous and trigram score of poss trans.
	// just need trigram score of the words of the start of current phrase	
	for (iterLM = lmListInitial.begin() ; iterLM != lmListInitial.end() ; ++iterLM)
	{
		const LanguageModel &languageModel = **iterLM;
		FactorType factorType = languageModel.GetFactorType();
		size_t nGramOrder			= languageModel.GetNGramOrder();
		float lmScore;

		if(m_currTargetWordsRange.GetWordsCount() > 0) //non-empty target phrase
		{
			// 1st n-gram
			vector<const Factor*> contextFactor(nGramOrder);
			size_t index = 0;
			for (int currPos = (int) startPos - (int) nGramOrder + 1 ; currPos <= (int) startPos ; currPos++)
			{
				if (currPos >= 0)
					contextFactor[index++] = GetFactor(currPos, factorType);
				else			
					contextFactor[index++] = languageModel.GetSentenceStart();
			}
			lmScore	= languageModel.GetValue(contextFactor);
			//cout<<"context factor: "<<languageModel.GetValue(contextFactor)<<endl;

			// main loop
			size_t endPos = std::min(startPos + nGramOrder - 2
															, m_currTargetWordsRange.GetEndPos());
			for (size_t currPos = startPos + 1 ; currPos <= endPos ; currPos++)
			{
				// shift all args down 1 place
				for (size_t i = 0 ; i < nGramOrder - 1 ; i++)
					contextFactor[i] = contextFactor[i + 1];
	
				// add last factor
				contextFactor.back() = GetFactor(currPos, factorType);
	
				lmScore	+= languageModel.GetValue(contextFactor);
				//cout<<"context factor: "<<languageModel.GetValue(contextFactor)<<endl;		
			}

			// end of sentence
			if (m_sourceCompleted.IsComplete())
			{
				const size_t size = GetSize();
				contextFactor.back() = languageModel.GetSentenceEnd();
	
				for (size_t i = 0 ; i < nGramOrder - 1 ; i ++)
				{
					int currPos = size - nGramOrder + i + 1;
					if (currPos < 0)
						contextFactor[i] = languageModel.GetSentenceStart();
					else
						contextFactor[i] = GetFactor((size_t)currPos, factorType);
				}
				lmScore	+= languageModel.GetValue(contextFactor);
			}
		}
		else lmScore = 0; //the score associated with dropping source words is not part of the language model
		
		m_score[ScoreType::LanguageModelScore] += lmScore * languageModel.GetWeight();
#ifdef N_BEST
		size_t lmId = languageModel.GetId();
		m_lmScoreComponent[lmId] += lmScore;
#endif
	}
}


//void Hypothesis::CalcScore(const LMList		&lmListInitial
//													, const LMList	&lmListEnd
//													, float weightDistortion
//													, float weightWordPenalty
//													, const SquareMatrix &futureScore
//													, const Sentence &source
//													, LexicalReordering *m_lexreorder) 
//=======
void Hypothesis::CalcDistortionScore()

{
	const WordsRange &prevRange = m_prevHypo->GetCurrSourceWordsRange()
								, &currRange	= GetCurrSourceWordsRange();
				
	if (prevRange.GetWordsCount() == 0)
	{ // 1st hypothesis with translated phrase. NOT the seed hypo.
		m_score[ScoreType::Distortion]	=  - (float) currRange.GetStartPos();
	}
	else
	{ // add distortion score of current translated phrase to
		// distortions scores of all previous partial translations
		m_score[ScoreType::Distortion]	-=  (float) currRange.CalcDistortion(prevRange) ;
	}
}

/***
 * calculate the logarithm of our total translation score (sum up components)
 */
void Hypothesis::CalcScore(const StaticData& staticData, const SquareMatrix &futureScore) 
{
	// DISTORTION COST
	CalcDistortionScore();
	
	// LANGUAGE MODEL COST
	CalcLMScore(staticData.GetLanguageModel(Initial), staticData.GetLanguageModel(Other));

	// WORD PENALTY
	m_score[ScoreType::WordPenalty] = - (float) GetSize();

	// FUTURE COST
	CalcFutureScore(futureScore);

	
	//LEXICAL REORDERING COST
	LexicalReordering *m_lexReorder = staticData.GetLexReorder();
	if(m_lexReorder == NULL)
		{
		//we're not scoring lexical reordering, so we add in a zero
		m_score[ScoreType::LexicalReordering] = 0;
		}
	else
		{
		//we're scoring lexical reordering, so we add in the score
		m_score[ScoreType::LexicalReordering] =  m_lexReorder->CalcScore(this);
		}



	

	// TOTAL COST
	m_score[ScoreType::Total] = m_score[ScoreType::PhraseTrans]
								+ m_score[ScoreType::Generation]			
								+ m_score[ScoreType::LanguageModelScore]
								+ m_score[ScoreType::Distortion]					* staticData.GetWeightDistortion()
								+ m_score[ScoreType::WordPenalty]					* staticData.GetWeightWordPenalty()
								+ m_score[ScoreType::FutureScoreEnum]
								+ m_score[ScoreType::LexicalReordering];
}

void Hypothesis::CalcFutureScore(const SquareMatrix &futureScore)
{
	const size_t maxSize= numeric_limits<size_t>::max();
	size_t	start				= maxSize;
	m_score[ScoreType::FutureScoreEnum]	= 0;
	for(size_t currPos = 0 ; currPos < m_sourceCompleted.GetSize() ; currPos++) 
	{
		if(m_sourceCompleted.GetValue(currPos) == 0 && start == maxSize)
		{
			start = currPos;
		}
		if(m_sourceCompleted.GetValue(currPos) == 1 && start != maxSize) 
		{
//			m_score[ScoreType::FutureScoreEnum] += futureScore[start][currPos - 1];
			m_score[ScoreType::FutureScoreEnum] += futureScore.GetScore(start, currPos - 1);
			start = maxSize;
		}
	}
	if (start != maxSize)
	{
//		m_score[ScoreType::FutureScoreEnum] += futureScore[start][m_sourceCompleted.GetSize() - 1];
		m_score[ScoreType::FutureScoreEnum] += futureScore.GetScore(start, m_sourceCompleted.GetSize() - 1);
	}
	
	//	m_score[ScoreType::FutureScoreEnum] += m_sourceCompleted.GetFutureCosts(m_currSourceWordsRange.GetEndPos()) * StaticData::Instance()->GetWeightDistortion();
	
}


int Hypothesis::GetId()const{
	return m_id;
}

const Hypothesis* Hypothesis::GetPrevHypo()const{
	return m_prevHypo;
}

/**
 * print hypothesis information for pharaoh-style logging
 */
void Hypothesis::PrintHypothesis(const InputType &source, float weightDistortion, float weightWordPenalty) const{
  cout<<"creating hypothesis "<< m_id <<" from "<< m_prevHypo->m_id<<" ( ";
  int end = m_prevHypo->m_targetPhrase.GetSize()-1;
  int start = end-1;
  if ( start < 0 ) start = 0;
  if ( m_prevHypo->m_currTargetWordsRange.GetStartPos() == NOT_FOUND ) {
    cout << "<s> ";
  }
  else {
    cout << "... ";
  }
  if (end>=0) {
    WordsRange range(start, end);
    cout << m_prevHypo->m_targetPhrase.GetSubString(range) << " ";
  }
  cout<<")"<<endl;
	cout<<"\tbase score "<< (m_prevHypo->m_score[ScoreType::Total] - m_prevHypo->m_score[ScoreType::FutureScoreEnum]) <<endl;
	cout<<"\tcovering "<<m_currSourceWordsRange.GetStartPos()<<"-"<<m_currSourceWordsRange.GetEndPos()<<": "<< source.GetSubString(m_currSourceWordsRange)  <<endl;
	cout<<"\ttranslated as: "<<m_targetPhrase<<" => translation cost "<<m_score[ScoreType::PhraseTrans];
  if (m_wordDeleted) cout <<"   word_deleted"; 
  cout<<endl;
	cout<<"\tdistance: "<<GetCurrSourceWordsRange().CalcDistortion(m_prevHypo->GetCurrSourceWordsRange()) << " => distortion cost "<<(m_score[ScoreType::Distortion]*weightDistortion)<<endl;
	cout<<"\tlanguage model cost "<<m_score[ScoreType::LanguageModelScore]<<endl;
	cout<<"\tword penalty "<<(m_score[ScoreType::WordPenalty]*weightWordPenalty)<<endl;
	cout<<"\tscore "<<m_score[ScoreType::Total] - m_score[ScoreType::FutureScoreEnum]<<" + future cost "<<m_score[ScoreType::FutureScoreEnum]<<" = "<<m_score[ScoreType::Total]<<endl;
#if N_BEST
  cout<<"\tweighted feature scores: " << this->GetTranslationScoreComponent() << endl;
#endif
	//PrintLMScores();
}


 
// friend
ostream& operator<<(ostream& out, const Hypothesis& hypothesis)
{	
	hypothesis.ToStream(out);
	// words bitmap
	out << "[" << hypothesis.m_sourceCompleted << "] ";
	
	// scores
	out << " [" << hypothesis.GetScore( static_cast<ScoreType::ScoreType>(0));
	for (size_t i = 1 ; i < NUM_SCORES ; i++)
	{
		out << "," << hypothesis.GetScore( static_cast<ScoreType::ScoreType>(i));
	}
	out << "]";
#ifdef N_BEST
		out << " " << hypothesis.GetTranslationScoreComponent();
		out << " " << hypothesis.GetGenerationScoreComponent();
#endif
	return out;
}
