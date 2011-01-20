/*
 *  CFunctions.h
 *  moses
 *
 *  Created by Hieu Hoang on 06/02/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <vector>
#include <string>

int LoadModel(const char *appPath, const char *iniPath, char *source, char *target, char *description);
int TranslateSentence(const char *rawInput, char *output);
std::vector<std::string> SegmentSentenceAndWord(const std::string &sentence);


