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

void Tokenize(OnDiskPt::SourcePhrase &sourcePhrase, OnDiskPt::TargetPhrase &targetPhrase
							, char *line, OnDiskPt::OnDiskWrapper &onDiskWrapper
							, std::string &sourceStr, int numScores
							, std::vector<float> &misc);
std::string Sort(const std::string &filePath, bool doSort);
void InsertTargetNonTerminals(std::vector<std::string> &sourceToks, const std::vector<std::string> &targetToks, const AlignType &alignments);
void SortAlign(AlignType &alignments);
bool Flush(const OnDiskPt::SourcePhrase *prevSource, const OnDiskPt::SourcePhrase *currSource);

