#pragma once
/*
 *  Main.h
 *  CreateOnDisk
 *
 *  Created by Hieu Hoang on 31/12/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#include <string>
#include "../../OnDiskPt/src/SourcePhrase.h"
#include "../../OnDiskPt/src/TargetPhrase.h"

typedef std::pair<size_t, size_t>  AlignPair;
typedef std::vector<AlignPair> AlignType;

void Tokenize(OnDiskPt::Phrase &phrase
							, const std::string &token, bool addSourceNonTerm, bool addTargetNonTerm
							, OnDiskPt::OnDiskWrapper &onDiskWrapper);
void Tokenize(OnDiskPt::SourcePhrase &sourcePhrase, OnDiskPt::TargetPhrase &targetPhrase
							, char *line, OnDiskPt::OnDiskWrapper &onDiskWrapper
							, int numScores
							, std::vector<float> &misc);

void InsertTargetNonTerminals(std::vector<std::string> &sourceToks, const std::vector<std::string> &targetToks, const AlignType &alignments);
void SortAlign(AlignType &alignments);
bool Flush(const OnDiskPt::SourcePhrase *prevSource, const OnDiskPt::SourcePhrase *currSource);

