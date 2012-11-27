# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -x c++ -D LM_INTERNAL -D LM_IRST -DTRACE_ENABLE  -D _FILE_OFFSET_BITS=64 -D _LARGE_FILES -D __USE_FILE_OFFSET64 -D _USE_FILE_OFFSET64  -I irstlm/src
LOCAL_LDLIBS += -lz -ldl

LOCAL_MODULE    := moses-jni
LOCAL_SRC_FILES := MosesUIInterface.cpp \
				irstlm/src/cmd.c		 \
				irstlm/src/lmmacro.cpp		 \
				irstlm/src/ngramcache.cpp \
				irstlm/src/shiftlm.cpp \
				irstlm/src/lmtable.cpp		 \
				irstlm/src/ngramtable.cpp \
				irstlm/src/timer.cpp \
				irstlm/src/dictionary.cpp \
				irstlm/src/mdiadapt.cpp \
				irstlm/src/htable.cpp		 \
				irstlm/src/mempool.cpp		 \
				irstlm/src/normcache.cpp \
				irstlm/src/util.cpp \
				irstlm/src/interplm.cpp		 \
				irstlm/src/mfstream.cpp	 \
				irstlm/src/mixture.cpp		 \
				irstlm/src/linearlm.cpp		 \
				irstlm/src/n_gram.cpp	 \
				OnDiskPt/src/OnDiskWrapper.cpp \
				OnDiskPt/src/TargetPhrase.cpp \
				OnDiskPt/src/Phrase.cpp \
				OnDiskPt/src/TargetPhraseCollection.cpp \
				OnDiskPt/src/PhraseNode.cpp \
				OnDiskPt/src/Vocab.cpp \
				OnDiskPt/src/SourcePhrase.cpp \
				OnDiskPt/src/Word.cpp \
				tokenizer/Collation.cpp \
				tokenizer/Tokenizer.cpp \
        moses/src/AlignmentInfo.cpp \
        moses/src/BilingualDynSuffixArray.cpp \
        moses/src/BitmapContainer.cpp \
        moses/src/ChartTranslationOption.cpp \
        moses/src/ChartTranslationOptionList.cpp \
        moses/src/ConfusionNet.cpp \
        moses/src/DecodeFeature.cpp \
        moses/src/DecodeGraph.cpp \
        moses/src/DecodeStep.cpp \
        moses/src/DecodeStepGeneration.cpp \
        moses/src/DecodeStepTranslation.cpp \
        moses/src/Dictionary.cpp \
        moses/src/DotChart.cpp \
        moses/src/DotChartOnDisk.cpp \
        moses/src/DummyScoreProducers.cpp \
				moses/src/DynSAInclude/file.cpp \
				moses/src/DynSAInclude/vocab.cpp \
        moses/src/DynSuffixArray.cpp \
        moses/src/FFState.cpp \
        moses/src/Factor.cpp \
        moses/src/FactorCollection.cpp \
        moses/src/FactorTypeSet.cpp \
        moses/src/FeatureFunction.cpp \
        moses/src/FloydWarshall.cpp \
        moses/src/GenerationDictionary.cpp \
        moses/src/GlobalLexicalModel.cpp \
        moses/src/hash.cpp \
        moses/src/Hypothesis.cpp \
        moses/src/HypothesisStack.cpp \
        moses/src/HypothesisStackCubePruning.cpp \
        moses/src/HypothesisStackNormal.cpp \
        moses/src/InputFileStream.cpp \
        moses/src/InputType.cpp \
        moses/src/LMList.cpp \
        moses/src/LVoc.cpp \
        moses/src/LanguageModel.cpp \
        moses/src/LanguageModelFactory.cpp \
        moses/src/LanguageModelImplementation.cpp \
        moses/src/LanguageModelInternal.cpp \
        moses/src/LanguageModelIRST.cpp \
        moses/src/LanguageModelJoint.cpp \
        moses/src/LanguageModelMultiFactor.cpp \
        moses/src/LanguageModelRemote.cpp \
        moses/src/LanguageModelSingleFactor.cpp \
        moses/src/LanguageModelSkip.cpp \
        moses/src/LexicalReordering.cpp \
        moses/src/LexicalReorderingState.cpp \
        moses/src/LexicalReorderingTable.cpp \
        moses/src/Manager.cpp \
        moses/src/NGramCollection.cpp \
        moses/src/NGramNode.cpp \
        moses/src/PCNTools.cpp \
        moses/src/Parameter.cpp \
        moses/src/PartialTranslOptColl.cpp \
        moses/src/Phrase.cpp \
        moses/src/PhraseDictionary.cpp \
        moses/src/PhraseDictionaryDynSuffixArray.cpp \
        moses/src/PhraseDictionaryMemory.cpp \
        moses/src/PhraseDictionaryNode.cpp \
        moses/src/PhraseDictionaryNodeSCFG.cpp \
        moses/src/PhraseDictionaryTree.cpp \
        moses/src/PhraseDictionaryTreeAdaptor.cpp \
        moses/src/PhraseDictionarySCFG.cpp \
        moses/src/PhraseDictionarySCFGChart.cpp \
        moses/src/PhraseDictionaryOnDisk.cpp \
        moses/src/PhraseDictionaryOnDiskChart.cpp \
        moses/src/PrefixTreeMap.cpp \
        moses/src/ReorderingConstraint.cpp \
        moses/src/ReorderingStack.cpp \
        moses/src/ScoreComponentCollection.cpp \
        moses/src/ScoreIndexManager.cpp \
        moses/src/ScoreProducer.cpp \
        moses/src/Search.cpp \
        moses/src/SearchCubePruning.cpp \
        moses/src/SearchNormal.cpp \
        moses/src/Sentence.cpp \
        moses/src/SentenceStats.cpp \
        moses/src/SquareMatrix.cpp \
        moses/src/StaticData.cpp \
        moses/src/TargetPhrase.cpp \
        moses/src/TargetPhraseCollection.cpp \
        moses/src/Timer.cpp \
        moses/src/TranslationOption.cpp \
        moses/src/TranslationOptionCollection.cpp \
        moses/src/TranslationOptionCollectionConfusionNet.cpp \
        moses/src/TranslationOptionCollectionText.cpp \
        moses/src/TranslationOptionList.cpp \
        moses/src/TranslationSystem.cpp \
        moses/src/TreeInput.cpp \
        moses/src/TrellisPath.cpp \
        moses/src/TrellisPathCollection.cpp \
        moses/src/UserMessage.cpp \
        moses/src/Util.cpp \
        moses/src/Word.cpp \
        moses/src/WordLattice.cpp \
        moses/src/WordsBitmap.cpp \
        moses/src/WordsRange.cpp \
        moses/src/WordConsumed.cpp \
        moses/src/XmlOption.cpp \


#LOCAL_CPPFLAGS += -fexceptions -fno-rtti

include $(BUILD_SHARED_LIBRARY)
