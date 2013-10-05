#pragma once
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

/* Created by Rohit Gupta, CDAC, Mumbai, India on 18 July, 2012*/

#include <string>
#include <vector>

namespace MosesTraining
{
enum REO_MODEL_TYPE {REO_MSD, REO_MSLR, REO_MONO};
enum REO_POS {LEFT, RIGHT, DLEFT, DRIGHT, UNKNOWN};


class PhraseExtractionOptions
{

public:
  const int maxPhraseLength;
private:
  bool allModelsOutputFlag;
  bool wordModel;
  REO_MODEL_TYPE wordType;
  bool phraseModel;
  REO_MODEL_TYPE phraseType;
  bool hierModel;
  REO_MODEL_TYPE hierType;
  bool orientationFlag;
  bool translationFlag;
  bool includeSentenceIdFlag; //include sentence id in extract file
  bool onlyOutputSpanInfo;
  bool gzOutput;
  std::string instanceWeightsFile; //weights for each sentence
  bool flexScoreFlag;

public:
  std::vector<std::string> placeholders;
  bool debug;

  PhraseExtractionOptions(const int initmaxPhraseLength):
    maxPhraseLength(initmaxPhraseLength),
    allModelsOutputFlag(false),
    wordModel(false),
    wordType(REO_MSD),
    phraseModel(false),
    phraseType(REO_MSD),
    hierModel(false),
    hierType(REO_MSD),
    orientationFlag(false),
    translationFlag(true),
    includeSentenceIdFlag(false),
    onlyOutputSpanInfo(false),
    gzOutput(false),
	flexScoreFlag(false), 
	debug(false)
{}

  //functions for initialization of options
  void initAllModelsOutputFlag(const bool initallModelsOutputFlag) {
    allModelsOutputFlag=initallModelsOutputFlag;
  }
  void initWordModel(const bool initwordModel) {
    wordModel=initwordModel;
  }
  void initWordType(REO_MODEL_TYPE initwordType ) {
    wordType=initwordType;
  }
  void initPhraseModel(const bool initphraseModel ) {
    phraseModel=initphraseModel;
  }
  void initPhraseType(REO_MODEL_TYPE initphraseType) {
    phraseType=initphraseType;
  }
  void initHierModel(const bool inithierModel) {
    hierModel=inithierModel;
  }
  void initHierType(REO_MODEL_TYPE inithierType) {
    hierType=inithierType;
  }
  void initOrientationFlag(const bool initorientationFlag) {
    orientationFlag=initorientationFlag;
  }
  void initTranslationFlag(const bool inittranslationFlag) {
    translationFlag=inittranslationFlag;
  }
  void initIncludeSentenceIdFlag(const bool initincludeSentenceIdFlag) {
    includeSentenceIdFlag=initincludeSentenceIdFlag;
  }
  void initOnlyOutputSpanInfo(const bool initonlyOutputSpanInfo) {
    onlyOutputSpanInfo= initonlyOutputSpanInfo;
  }
  void initGzOutput (const bool initgzOutput) {
    gzOutput= initgzOutput;
  }
  void initInstanceWeightsFile(const char* initInstanceWeightsFile) {
    instanceWeightsFile = std::string(initInstanceWeightsFile);
  }
  void initFlexScoreFlag(const bool initflexScoreFlag) {
    flexScoreFlag=initflexScoreFlag;
  }

  // functions for getting values
  bool isAllModelsOutputFlag() const {
    return allModelsOutputFlag;
  }
  bool isWordModel() const {
    return wordModel;
  }
  REO_MODEL_TYPE isWordType() const {
    return wordType;
  }
  bool isPhraseModel() const {
    return phraseModel;
  }
  REO_MODEL_TYPE isPhraseType() const {
    return phraseType;
  }
  bool isHierModel() const {
    return hierModel;
  }
  REO_MODEL_TYPE isHierType() const {
    return hierType;
  }
  bool isOrientationFlag() const {
    return orientationFlag;
  }
  bool isTranslationFlag() const {
    return translationFlag;
  }
  bool isIncludeSentenceIdFlag() const {
    return includeSentenceIdFlag;
  }
  bool isOnlyOutputSpanInfo() const {
    return onlyOutputSpanInfo;
  }
  bool isGzOutput () const {
    return gzOutput;
  }
  std::string getInstanceWeightsFile() const {
    return instanceWeightsFile;
  }
  bool isFlexScoreFlag() const {
    return flexScoreFlag;
  }
};

}

