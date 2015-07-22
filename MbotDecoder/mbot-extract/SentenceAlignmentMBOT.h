// modified for mbot support
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

#pragma once
#ifndef SENTENCEALIGNMENTMBOT_H_INCLUDED_
#define SENTENCEALIGNMENTMBOT_H_INCLUDED_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "SyntaxTree.h"
#include "MBOTExtractionOptions.h"

class SentenceAlignmentMBOT
{
public:
  SyntaxTree targetTree;
  SyntaxTree sourceTree;
  std::set<std::string> & m_targetLabelCollection;
  std::set<std::string> & m_sourceLabelCollection;
  std::map<std::string, int> & m_targetTopLabelCollection;
  std::map<std::string, int> & m_sourceTopLabelCollection;
  std::vector<std::string> target;
  std::vector<std::string> source;
  std::vector<int> alignedCountS;
  std::vector<int> alignedCountT;
  std::vector<std::vector<int> > alignedToT;
  std::vector<std::vector<int> > alignedToS;
  int sentenceID;
  const MBOTExtractionOptions & m_options;

 SentenceAlignmentMBOT(std::set<std::string> & tgtLabelColl,
                       std::set<std::string> & srcLabelColl,
                       std::map<std::string,int> & tgtTopLabelColl,
                       std::map<std::string,int> & srcTopLabelColl,
                       const MBOTExtractionOptions & options)
    : m_targetLabelCollection(tgtLabelColl)
    , m_sourceLabelCollection(srcLabelColl)
    , m_targetTopLabelCollection(tgtTopLabelColl)
    , m_sourceTopLabelCollection(srcTopLabelColl)
    , m_options(options)
  {}

  virtual ~SentenceAlignmentMBOT() {}

  bool processTargetSentence(const char *, int);

  bool processSourceSentence(const char *, int); 

  bool create(char targetString[], char sourceString[], 
	      char alignmentString[], int sentenceID);
};

#endif
