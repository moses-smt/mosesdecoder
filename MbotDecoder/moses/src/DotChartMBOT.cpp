// $Id: DotChartMBOT.cpp,v 1.1.1.1 2013/01/06 16:54:18 braunefe Exp $
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang

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

#include "DotChartMBOT.h"

namespace Moses
{

//Output for more detailed translation report
std::ostream &operator<<(std::ostream &out, const DottedRuleMBOT &rule)
{
  //
  out << "----------------" << std::endl;
  out << "MBOT Dotted Rule" << std::endl;
  out << "----------------" << std::endl;
  if (!rule.IsRootMBOT()) {
    out << rule.GetWordsRangeMBOT() << "=" <<  rule.GetSourceWordMBOT() << " ";
    out << "Size of Cell Label : " << rule.GetChartCellLabelMBOT().GetLabelMBOT().size() << std::endl;
    out << "Cell Labels : " << rule.GetChartCellLabelMBOT().GetLabelMBOT().size() << std::endl;
    std::vector<Word> words = rule.GetChartCellLabelMBOT().GetLabelMBOT();
    std::vector<Word> :: iterator itr_words;
    size_t counter = 1;
        for(itr_words = words.begin(); itr_words != words.end(); itr_words++)
        {
            out << *itr_words << "(" << counter++ << ")";
        }
        out << std::endl;

        out << "Coverages" << std::endl;
        std::vector<WordsRange> coverages = rule.GetChartCellLabelMBOT().GetCoverageMBOT();
        std::vector<WordsRange>::const_iterator itr_coverage;
        for(itr_coverage = coverages.begin(); itr_coverage != coverages.end(); itr_coverage++)
        {
            out << *itr_coverage;
        }
        out << std::endl;
        }

    if (!rule.GetPrevMBOT()->IsRootMBOT()) {
      out << " " << *rule.GetPrevMBOT() << std::endl;
    }
  return out;
}
}
