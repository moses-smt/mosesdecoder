/*
 * Manager.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "Manager.h"
#include "PhraseTable.h"
#include "System.h"
#include "SearchNormal.h"
#include "TargetPhrases.h"
#include "TargetPhrase.h"
#include "InputPaths.h"
#include "InputPath.h"
#include "moses/Range.h"

using namespace std;

Manager::Manager(System &system, const std::string &inputStr)
:m_pool(&system.GetManagerPool())
,m_system(system)
,m_initRange(NOT_FOUND, NOT_FOUND)
,m_initPhrase(system.GetManagerPool(), system, 0)
{
	Moses::FactorCollection &vocab = system.GetVocab();

	m_input = Phrase::CreateFromString(GetPool(), vocab, inputStr);
	m_inputPaths.Init(*m_input, system);

	const std::vector<const PhraseTable*> &pts = system.GetMapping();
	for (size_t i = 0; i < pts.size(); ++i) {
		const PhraseTable &pt = *pts[i];
		//cerr << "Looking up from " << pt.GetName() << endl;
		pt.Lookup(*this, m_inputPaths);
	}

	m_stacks.resize(m_input->GetSize() + 1);
	m_bitmaps = new Moses::Bitmaps(m_input->GetSize(), vector<bool>(0));
	m_search = new SearchNormal(*this, m_stacks);
}

Manager::~Manager() {
	delete m_bitmaps;
	delete m_search;
	delete m_futureScore;

	GetPool().Reset();
}

const Hypothesis *Manager::GetBestHypothesis() const
{
	return m_search->GetBestHypothesis();
}

void Manager::Decode()
{
	const Moses::Bitmap &initBitmap = m_bitmaps->GetInitialBitmap();
	Hypothesis *initHypo = new (GetPool().Allocate<Hypothesis>()) Hypothesis(*this, m_initPhrase, m_initRange, initBitmap);
	initHypo->EmptyHypothesisState(*m_input);

	StackAdd stackAdded = m_stacks[0].Add(initHypo);
	assert(stackAdded.added);

	for (size_t i = 0; i < m_stacks.size(); ++i) {
		m_search->Decode(i);
	}
}

void Manager::CalcFutureScore()
{
	size_t size = m_input->GetSize();
	m_futureScore = new Moses::SquareMatrix(size);
	m_futureScore->InitTriangle(-numeric_limits<SCORE>::infinity());

    // walk all the translation options and record the cheapest option for each span
	BOOST_FOREACH(const InputPath &path, m_inputPaths) {
		const Moses::Range &range = path.GetRange();
		const std::vector<TargetPhrases::shared_const_ptr> &allTps = path.GetTargetPhrases();
		SCORE bestScore = -numeric_limits<SCORE>::infinity();

	    BOOST_FOREACH(const TargetPhrases::shared_const_ptr &tpsSharedPtr, allTps) {
     	  const TargetPhrases *tps = tpsSharedPtr.get();
     	  if (tps) {
     		 BOOST_FOREACH(const TargetPhrase *tp, *tps) {
     			SCORE score = tp->GetFutureScore();
 				if (score > bestScore) {
 					bestScore = score;
 				}
     		 }
     	  }
	    }
	    m_futureScore->SetScore(range.GetStartPos(), range.GetEndPos(), bestScore);
	}

	  // now fill all the cells in the strictly upper triangle
	  //   there is no way to modify the diagonal now, in the case
	  //   where no translation option covers a single-word span,
	  //   we leave the +inf in the matrix
	  // like in chart parsing we want each cell to contain the highest score
	  // of the full-span trOpt or the sum of scores of joining two smaller spans

	  for(size_t colstart = 1; colstart < size ; colstart++) {
	    for(size_t diagshift = 0; diagshift < size-colstart ; diagshift++) {
	      size_t sPos = diagshift;
	      size_t ePos = colstart+diagshift;
	      for(size_t joinAt = sPos; joinAt < ePos ; joinAt++)  {
	        float joinedScore = m_futureScore->GetScore(sPos, joinAt)
	                            + m_futureScore->GetScore(joinAt+1, ePos);
	        // uncomment to see the cell filling scheme
	        // TRACE_ERR("[" << sPos << "," << ePos << "] <-? ["
	        // 	  << sPos << "," << joinAt << "]+["
	        // 	  << joinAt+1 << "," << ePos << "] (colstart: "
	        // 	  << colstart << ", diagshift: " << diagshift << ")"
	        // 	  << endl);

	        if (joinedScore > m_futureScore->GetScore(sPos, ePos))
	          m_futureScore->SetScore(sPos, ePos, joinedScore);
	      }
	    }
	  }

}

