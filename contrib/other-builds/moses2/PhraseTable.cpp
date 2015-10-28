/*
 * PhraseTable.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "PhraseTable.h"
#include "InputPaths.h"

using namespace std;

PhraseTable::PhraseTable(size_t startInd, const std::string &line)
:StatelessFeatureFunction(startInd, line)
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

  }
  else {
	  StatelessFeatureFunction::SetParameter(key, value);
  }
}

void PhraseTable::Lookup(InputPaths &inputPaths) const
{
	  BOOST_FOREACH(InputPath &path, inputPaths) {
			const SubPhrase &phrase = path.GetSubPhrase();
			const TargetPhrases *tps = Lookup(path);
			cerr << "path=" << path << endl;
			cerr << "tps=" << tps << endl;
			if (tps) {
				cerr << *tps << endl;
			}

			path.AddTargetPhrases(*this, tps);
	  }

}

const TargetPhrases *PhraseTable::Lookup(InputPath &inputPath) const
{
  UTIL_THROW2("Not implemented");
}

