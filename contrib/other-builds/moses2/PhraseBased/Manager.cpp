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
#include "TargetPhraseImpl.h"
#include "InputPath.h"
#include "Normal/Search.h"
#include "CubePruningMiniStack/Search.h"
/*
#include "CubePruningPerMiniStack/Search.h"
#include "CubePruningPerBitmap/Search.h"
#include "CubePruningCardinalStack/Search.h"
#include "CubePruningBitmapStack/Search.h"
*/
#include "../System.h"
#include "../TargetPhrases.h"
#include "../Phrase.h"
#include "../InputPaths.h"
#include "../Sentence.h"
#include "../TranslationModel/PhraseTable.h"
#include "../legacy/Range.h"

using namespace std;

namespace Moses2
{
Manager::Manager(System &sys, const TranslationTask &task, const std::string &inputStr, long translationId)
:ManagerBase(sys, task, inputStr, translationId)
{}

Manager::~Manager() {

	delete m_search;
	delete m_bitmaps;
}

void Manager::Init()
{
	// init pools etc
	InitPools();
	ParseInput(false);

	m_bitmaps = new Bitmaps(GetPool());

	const PhraseTable &firstPt = *system.featureFunctions.m_phraseTables[0];
	m_initPhrase = new (GetPool().Allocate<TargetPhraseImpl>()) TargetPhraseImpl(GetPool(), firstPt, system, 0);

	m_inputPaths.Init(*m_input, *this);

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
	case CubePruning:
	case CubePruningMiniStack:
		m_search = new NSCubePruningMiniStack::Search(*this);
		break;
/*
 	case CubePruningPerMiniStack:
		m_search = new NSCubePruningPerMiniStack::Search(*this);
		break;
	case CubePruningPerBitmap:
		m_search = new NSCubePruningPerBitmap::Search(*this);
		break;
	case CubePruningCardinalStack:
		m_search = new NSCubePruningCardinalStack::Search(*this);
		break;
	case CubePruningBitmapStack:
		m_search = new NSCubePruningBitmapStack::Search(*this);
		break;
*/
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
	m_estimatedScores = new (GetPool().Allocate<EstimatedScores>()) EstimatedScores(GetPool(), size);
	m_estimatedScores->InitTriangle(-numeric_limits<SCORE>::infinity());

    // walk all the translation options and record the cheapest option for each span
	BOOST_FOREACH(const InputPathBase *path, m_inputPaths) {
		const Range &range = path->range;
		SCORE bestScore = -numeric_limits<SCORE>::infinity();

  	    size_t numPt = system.mappings.size();
  	    for (size_t i = 0; i < numPt; ++i) {
 	      const TargetPhrases *tps = static_cast<const InputPath*>(path)->targetPhrases[i];
     	  if (tps) {
     		 BOOST_FOREACH(const TargetPhrase *tp, *tps) {
     			SCORE score = tp->GetFutureScore();
 				if (score > bestScore) {
 					bestScore = score;
 				}
     		 }
     	  }
	    }
	    m_estimatedScores->SetValue(range.GetStartPos(), range.GetEndPos(), bestScore);
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
	        float joinedScore = m_estimatedScores->GetValue(sPos, joinAt)
	                            + m_estimatedScores->GetValue(joinAt+1, ePos);
	        // uncomment to see the cell filling scheme
	        // TRACE_ERR("[" << sPos << "," << ePos << "] <-? ["
	        // 	  << sPos << "," << joinAt << "]+["
	        // 	  << joinAt+1 << "," << ePos << "] (colstart: "
	        // 	  << colstart << ", diagshift: " << diagshift << ")"
	        // 	  << endl);

	        if (joinedScore > m_estimatedScores->GetValue(sPos, ePos))
	          m_estimatedScores->SetValue(sPos, ePos, joinedScore);
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
		//cerr << "BEST TRANSLATION: " << *bestHypo;
	}
	else {
		if (system.outputHypoScore) {
			out << "0 ";
		}
		//cerr << "NO TRANSLATION " << m_input->GetTranslationId() << endl;
	}
	out << "\n";

	system.bestCollector.Write(m_input->GetTranslationId(), out.str());
	//cerr << endl;


}

}

