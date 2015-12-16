/*
 * Manager.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include <vector>
#include <sstream>
#include "Manager.h"
#include "SearchNormal.h"
#include "SearchNormalBatch.h"
#include "CubePruning/Search.h"
#include "../System.h"
#include "../TargetPhrases.h"
#include "../TargetPhrase.h"
#include "../InputPaths.h"
#include "../InputPath.h"
#include "../Sentence.h"
#include "../TranslationModel/PhraseTable.h"
#include "../legacy/Range.h"

using namespace std;

namespace Moses2
{

Manager::Manager(System &sys, const TranslationTask &task, const std::string &inputStr, long translationId)
:system(sys)
,task(task)
,m_inputStr(inputStr)
,m_translationId(translationId)
{}

Manager::~Manager() {
	m_bitmaps->Clear();

	delete m_search;
	delete m_estimatedScores;

	GetPool().Reset();
	GetHypoRecycle().Reset();
}

void Manager::Init()
{
	// init pools etc
	m_pool = &system.GetManagerPool();
	m_hypoRecycle = &system.GetHypoRecycler();
	m_bitmaps = &system.GetBitmaps();

	m_initPhrase = new (GetPool().Allocate<TargetPhrase>()) TargetPhrase(GetPool(), system, 0);


	// create input phrase obj
	FactorCollection &vocab = system.GetVocab();

	m_input = Sentence::CreateFromString(GetPool(), vocab, system, m_inputStr, m_translationId);
	m_inputPaths.Init(*m_input, system);

	const std::vector<const PhraseTable*> &pts = system.mappings;
	for (size_t i = 0; i < pts.size(); ++i) {
		const PhraseTable &pt = *pts[i];
		//cerr << "Looking up from " << pt.GetName() << endl;
		pt.Lookup(*this, m_inputPaths);
	}
	//m_inputPaths.DeleteUnusedPaths();

	CalcFutureScore();

	m_bitmaps->Init(m_input->GetSize(), vector<bool>(0));

	switch (system.searchAlgorithm) {
	case Normal:
		m_search = new SearchNormal(*this);
		break;
	case NormalBatch:
		m_search = new SearchNormalBatch(*this);
		break;
	case CubePruning:
		m_search = new NSCubePruning::Search(*this);
		break;
	default:
		cerr << "Unknown search algorithm" << endl;
		abort();
	}
}

void Manager::Decode()
{
	Init();
	m_search->Decode();
	OutputBest();
}

void Manager::CalcFutureScore()
{
	size_t size = m_input->GetSize();
	m_estimatedScores = new SquareMatrix(size);
	m_estimatedScores->InitTriangle(-numeric_limits<SCORE>::infinity());

    // walk all the translation options and record the cheapest option for each span
	BOOST_FOREACH(const InputPath &path, m_inputPaths) {
		const Range &range = path.range;
		const std::vector<const TargetPhrases*> &allTps = path.targetPhrases;
		SCORE bestScore = -numeric_limits<SCORE>::infinity();

	    BOOST_FOREACH(const TargetPhrases *tps, allTps) {
     	  if (tps) {
     		 BOOST_FOREACH(const TargetPhrase *tp, *tps) {
     			SCORE score = tp->GetFutureScore();
 				if (score > bestScore) {
 					bestScore = score;
 				}
     		 }
     	  }
	    }
	    m_estimatedScores->SetScore(range.GetStartPos(), range.GetEndPos(), bestScore);
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
	        float joinedScore = m_estimatedScores->GetScore(sPos, joinAt)
	                            + m_estimatedScores->GetScore(joinAt+1, ePos);
	        // uncomment to see the cell filling scheme
	        // TRACE_ERR("[" << sPos << "," << ePos << "] <-? ["
	        // 	  << sPos << "," << joinAt << "]+["
	        // 	  << joinAt+1 << "," << ePos << "] (colstart: "
	        // 	  << colstart << ", diagshift: " << diagshift << ")"
	        // 	  << endl);

	        if (joinedScore > m_estimatedScores->GetScore(sPos, ePos))
	          m_estimatedScores->SetScore(sPos, ePos, joinedScore);
	      }
	    }
	  }

	  //cerr << "Square matrix:" << endl;
	  //cerr << *m_estimatedScores << endl;
}

void Manager::OutputBest() const
{
	stringstream out;
    const Hypothesis *bestHypo = m_search->GetBestHypothesis();
	if (bestHypo) {
		if (system.outputHypoScore) {
			out << bestHypo->GetScores().GetTotalScore() << " ";
		}

		bestHypo->OutputToStream(out);
		cerr << "BEST TRANSLATION: " << *bestHypo;
	}
	else {
		cerr << "NO TRANSLATION";
	}
	out << "\n";

	system.bestCollector.Write(m_input->GetTranslationId(), out.str());
	cerr << endl;


}

}

