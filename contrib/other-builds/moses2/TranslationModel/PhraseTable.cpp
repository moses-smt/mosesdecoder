/*
 * PhraseTable.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "PhraseTable.h"
#include "../InputPaths.h"

using namespace std;

PhraseTable::PhraseTable(size_t startInd, const std::string &line)
:StatelessFeatureFunction(startInd, line)
,m_tableLimit(20) // default
{
	ReadParameters();
}

PhraseTable::~PhraseTable() {
	// TODO Auto-generated destructor stub
}

void PhraseTable::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
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

void PhraseTable::Lookup(const Manager &mgr, InputPaths &inputPaths) const
{
	  BOOST_FOREACH(InputPath &path, inputPaths) {
			const SubPhrase &phrase = path.subPhrase;
			TargetPhrases::shared_const_ptr tps = Lookup(mgr, path);

			/*
			cerr << "path=" << path.GetRange() << " ";
			cerr << "tps=" << tps << " ";
			if (tps.get()) {
				cerr << tps.get()->GetSize();
			}
			cerr << endl;
			*/

			path.AddTargetPhrases(*this, tps);
	  }

}

TargetPhrases::shared_const_ptr PhraseTable::Lookup(const Manager &mgr, InputPath &inputPath) const
{
  UTIL_THROW2("Not implemented");
}

void
PhraseTable::EvaluateInIsolation(const System &system,
		const Phrase &source, const TargetPhrase &targetPhrase,
		Scores &scores,
		Scores *estimatedScores) const
{

}
