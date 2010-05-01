/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2010 University of Edinburgh

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

#include "SentenceAlignmentWithSyntax.h"

#include <map>
#include <set>
#include <string>

#include "tables-core.h"
#include "XmlTree.h"

void SentenceAlignmentWithSyntax::processTargetSentence(const char * targetString)
{
    // tokenizing target (and potentially extract syntax spans)
    if (m_options.targetSyntax) {
        string targetStringCPP = string(targetString);
        ProcessAndStripXMLTags(targetStringCPP, targetTree, m_targetLabelCollection ,
                               m_targetTopLabelCollection);
        target = tokenize(targetStringCPP.c_str());
    }
    else {
        target = tokenize(targetString);
    }
}

void SentenceAlignmentWithSyntax::processSourceSentence(const char * sourceString)
{
    // tokenizing source (and potentially extract syntax spans)
    if (m_options.sourceSyntax) {
        string sourceStringCPP = string(sourceString);
        ProcessAndStripXMLTags(sourceStringCPP, sourceTree, m_sourceLabelCollection ,
                               m_sourceTopLabelCollection);
        source = tokenize(sourceStringCPP.c_str());
    }
    else {
        source = tokenize(sourceString);
    }
}
