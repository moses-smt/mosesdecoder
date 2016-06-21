/*
 * PhraseTable.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include <queue>
#include "PhraseTable.h"
#include "../legacy/Util2.h"
#include "../TypeDef.h"
#include "../InputType.h"
#include "../PhraseBased/Manager.h"
#include "../PhraseBased/InputPath.h"

using namespace std;

namespace Moses2
{

////////////////////////////////////////////////////////////////////////////
PhraseTable::PhraseTable(size_t startInd, const std::string &line) :
    StatelessFeatureFunction(startInd, line), m_tableLimit(20) // default
        , m_maxCacheSize(DEFAULT_MAX_TRANS_OPT_CACHE_SIZE)
{
  ReadParameters();
}

PhraseTable::~PhraseTable()
{
  // TODO Auto-generated destructor stub
}

void PhraseTable::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "cache-size") {
    m_maxCacheSize = Scan<size_t>(value);
  }
  else if (key == "path") {
    m_path = value;
  }
  else if (key == "input-factor") {

  }
  else if (key == "output-factor") {

  }
  else if (key == "table-limit") {
    m_tableLimit = Scan<size_t>(value);
  }
  else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

bool PhraseTable::SatisfyBackoff(const Manager &mgr, const InputPath &path) const
{
  const InputType &input = mgr.GetInput();
  if ((mgr.system.options.input.xml_policy == XmlExclusive)
	  && input.XmlOverlap(path.range.GetStartPos(), path.range.GetEndPos())) {
	  return false;
  }

	//cerr << GetName() << "=" << GetPtInd() << "=" << decodeGraphBackoff << endl;
	if (decodeGraphBackoff == 0) {
			// always lookup
			return true;
	}
	else if (decodeGraphBackoff == -1) {
			// lookup only if there's no existing rules
			return path.GetNumRules() ? false : true;
	}
	else if (path.range.GetNumWordsCovered() <= decodeGraphBackoff) {
			return path.GetNumRules() ? false : true;
	}

	return false;
}

void PhraseTable::Lookup(const Manager &mgr, InputPathsBase &inputPaths) const
{
  BOOST_FOREACH(InputPathBase *pathBase, inputPaths){
    InputPath *path = static_cast<InputPath*>(pathBase);

    if (SatisfyBackoff(mgr, *path)) {
		TargetPhrases *tpsPtr = tpsPtr = Lookup(mgr, mgr.GetPool(), *path);

		/*
		 cerr << "path=" << path.GetRange() << " ";
		 cerr << "tps=" << tps << " ";
		 if (tps.get()) {
		 cerr << tps.get()->GetSize();
		 }
		 cerr << endl;
		 */

		path->AddTargetPhrases(*this, tpsPtr);
    }
  }

}

TargetPhrases *PhraseTable::Lookup(const Manager &mgr, MemPool &pool,
    InputPath &inputPath) const
{
  UTIL_THROW2("Not implemented");
}

void PhraseTable::EvaluateInIsolation(MemPool &pool, const System &system,
    const Phrase<Moses2::Word> &source, const TargetPhrase<Moses2::Word> &targetPhrase, Scores &scores,
    SCORE *estimatedScore) const
{
}

void PhraseTable::EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
    const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
    SCORE *estimatedScore) const
{

}


void PhraseTable::CleanUpAfterSentenceProcessing()
{
}

// scfg
void PhraseTable::InitActiveChart(MemPool &pool, SCFG::InputPath &path) const
{
  UTIL_THROW2("Not implemented");
}


}

