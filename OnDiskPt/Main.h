#pragma once
// $Id$
/***********************************************************************
 Moses - factored phrase-based, hierarchical and syntactic language decoder
 Copyright (C) 2009 Hieu Hoang

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
#include <string>
#include "SourcePhrase.h"
#include "TargetPhrase.h"

typedef std::pair<size_t, size_t>  AlignPair;
typedef std::vector<AlignPair> AlignType;

OnDiskPt::WordPtr Tokenize(OnDiskPt::Phrase &phrase
                           , const std::string &token, bool addSourceNonTerm, bool addTargetNonTerm
                           , OnDiskPt::OnDiskWrapper &onDiskWrapper, int retSourceTarget);
OnDiskPt::PhrasePtr Tokenize(OnDiskPt::SourcePhrase &sourcePhrase, OnDiskPt::TargetPhrase &targetPhrase
                             , const std::string &lineStr, OnDiskPt::OnDiskWrapper &onDiskWrapper
                             , int numScores
                             , std::vector<float> &misc);

void InsertTargetNonTerminals(std::vector<std::string> &sourceToks, const std::vector<std::string> &targetToks, const AlignType &alignments);
void SortAlign(AlignType &alignments);
bool Flush(const OnDiskPt::SourcePhrase *prevSource, const OnDiskPt::SourcePhrase *currSource);

