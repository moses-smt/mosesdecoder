/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2011 University of Edinburgh

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

#include <sstream>

#include "AlignmentInfo.h"
#include "Gibbler.h"
#include "PhrasePairFeature.h"


using namespace Moses;
using namespace std;

namespace Josiah {

  const std::string PhrasePairFeature::PREFIX = "pp"; 

  PhrasePairFeature::PhrasePairFeature
    (Moses::FactorType sourceFactorId, Moses::FactorType targetFactorId) 
      : m_sourceFactorId(sourceFactorId), m_targetFactorId(targetFactorId) {}

  const Moses::Factor* PhrasePairFeature::getSourceFactor
    (const Moses::Word& word) const {
    return word[m_sourceFactorId];
  }

  const Moses::Factor* PhrasePairFeature::getTargetFactor
    (const Moses::Word& word) const {
    return word[m_targetFactorId];
  }



  void PhrasePairFeature::assign
    (const TranslationOption* option, FVector& scores) const {
    const TargetPhrase& target = option->GetTargetPhrase();
    const Phrase* source = option->GetSourcePhrase();
    const AlignmentInfo& align = target.GetAlignmentInfo();
//    cerr << source->GetStringRep(vector<FactorType>(1));
//    cerr << "|" <<  target.GetStringRep(vector<FactorType>(1));
    for (AlignmentInfo::const_iterator i = align.begin(); i != align.end(); ++i) {
      const Factor* sourceFactor = 
        getSourceFactor(source->GetWord(i->first));
      const Factor* targetFactor = 
        getTargetFactor(target.GetWord(i->second));
      ostringstream namestr;
      namestr << sourceFactor->GetString();
      namestr << ":";
      namestr << targetFactor->GetString();
      FName name(PhrasePairFeature::PREFIX,namestr.str());
      ++scores[name];
//      cerr << " " << name;
    }
//    cerr << endl;

  }

  

}


