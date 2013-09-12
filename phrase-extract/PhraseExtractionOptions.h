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

#pragma once
#ifndef PHRASEEXTRACTIONOPTIONS_H_INCLUDED_
#define PHRASEEXTRACTIONOPTIONS_H_INCLUDED_

namespace MosesTraining
{
enum REO_MODEL_TYPE {REO_MSD, REO_MSLR, REO_MONO};
enum REO_POS {LEFT, RIGHT, DLEFT, DRIGHT, UNKNOWN};


class PhraseExtractionOptions {
  
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
  bool sentenceIdFlag; //create extract file with sentence id
  bool onlyOutputSpanInfo;
  bool gzOutput;
  bool outputPsd;
  bool mtu;

public:  
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
            sentenceIdFlag(false),
            onlyOutputSpanInfo(false),
            gzOutput(false),
            outputPsd(false),
            mtu(false){}
 
    //functions for initialization of options
    void initAllModelsOutputFlag(const bool initallModelsOutputFlag){
        allModelsOutputFlag=initallModelsOutputFlag;
    }
    void initWordModel(const bool initwordModel){
        wordModel=initwordModel;
    }
    void initWordType(REO_MODEL_TYPE initwordType ){
        wordType=initwordType; 
    } 
    void initPhraseModel(const bool initphraseModel ){
        phraseModel=initphraseModel;  
    } 
    void initPhraseType(REO_MODEL_TYPE initphraseType){
        phraseType=initphraseType;
    }  
    void initHierModel(const bool inithierModel){
        hierModel=inithierModel;
    }
    void initHierType(REO_MODEL_TYPE inithierType){
        hierType=inithierType;
    }
    void initOrientationFlag(const bool initorientationFlag){
        orientationFlag=initorientationFlag;
    }
    void initTranslationFlag(const bool inittranslationFlag){
        translationFlag=inittranslationFlag;
    }
    void initSentenceIdFlag(const bool initsentenceIdFlag){
        sentenceIdFlag=initsentenceIdFlag;
    }
    void initOnlyOutputSpanInfo(const bool initonlyOutputSpanInfo){
        onlyOutputSpanInfo= initonlyOutputSpanInfo;
    } 
    void initGzOutput (const bool initgzOutput){
        gzOutput= initgzOutput;
    } 
    void initOutputPsd(bool initOutputPsd) {
      outputPsd = initOutputPsd;
    }
    void initMTU (const bool initMTU){
        mtu = initMTU;
    } 

    // functions for getting values
    bool isAllModelsOutputFlag(){
        return allModelsOutputFlag;
    }
    bool isOutputPsd(){
      return outputPsd;
    }
    bool isWordModel(){
        return wordModel;
    }
    REO_MODEL_TYPE isWordType(){
        return wordType; 
    } 
    bool isPhraseModel(){
        return phraseModel;  
    } 
    REO_MODEL_TYPE isPhraseType(){
        return phraseType;
    }  
    bool isHierModel(){
        return hierModel; 
    }
    REO_MODEL_TYPE isHierType(){
        return hierType;
    }
    bool isOrientationFlag(){
        return orientationFlag;
    }
    bool isTranslationFlag(){
        return translationFlag;
    }
    bool isSentenceIdFlag(){
        return sentenceIdFlag;
    }
    bool isOnlyOutputSpanInfo(){
        return onlyOutputSpanInfo;
    } 
    bool isGzOutput (){
        return gzOutput;
    } 
    bool isMTU (){
        return mtu;
    } 
};

}

#endif
