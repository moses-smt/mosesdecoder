// $Id$
// vim:tabstop=2

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
#include "SentenceStats.h"
#include "InputPath.h"
#include "TranslationOption.h"

using std::cout;
using std::endl;

namespace Moses
{
/***
 * to be called after decoding a sentence
 */
void SentenceStats::CalcFinalStats(const Hypothesis& bestHypo)
{
  //deleted words
  AddDeletedWords(bestHypo);
  //inserted words--not implemented yet 8/1 TODO
}

void SentenceStats::AddDeletedWords(const Hypothesis& hypo)
{
  //don't check either a null pointer or the empty initial hypothesis (if we were given the empty hypo, the null check will save us)
  if(hypo.GetPrevHypo() != NULL && hypo.GetPrevHypo()->GetCurrSourceWordsRange().GetNumWordsCovered() > 0) AddDeletedWords(*hypo.GetPrevHypo());

  if(hypo.GetPrevHypo() && hypo.GetCurrTargetWordsRange().GetNumWordsCovered() == 0) {

    m_deletedWords.push_back(&hypo.GetTranslationOption().GetInputPath().GetPhrase());
  }
}

}

