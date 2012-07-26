#ifndef psd_phrase_utils_h
#define psd_phrase_utils_h
#include "tables-core.h"
#include "FeatureExtractor.h"
#include <set>
#include <map>

#define LINE_MAX_LENGTH 10000

//DAMT HIERO NOTE : If used as interface for feature extraction code always use methods with Rule for hiero

using namespace std;

typedef MosesTraining::PhraseTable PhraseVocab;
typedef multimap< MosesTraining::PHRASE_ID, PSD::Translation > PhraseTranslations;

// Read Vocbulary for hiero rules
bool readRuleVocab(const char* vocabFile, MosesTraining::Vocabulary &wordVocab, PhraseVocab &vocab);

//Read rule table
bool readRules(const char *ptFile, MosesTraining::Vocabulary &srcWordVocab, MosesTraining::Vocabulary &tgtWordVocab, PhraseVocab &srcPhraseVocab, PhraseVocab &tgtPhraseVocab, PhraseTranslations &transTable);

//Make Vocabulary for hiero rules
MosesTraining::PHRASE makeRuleAndVoc(const string rule, MosesTraining::Vocabulary &wordVocab);

//Get ID of hiero rules
MosesTraining::PHRASE_ID getRuleID(const string rule, MosesTraining::Vocabulary &wordVocab, PhraseVocab &vocab);

//get target hiero rule, equivalent of get phrase. If use of
string getTargetRule(MosesTraining::PHRASE_ID labelid, MosesTraining::Vocabulary &tgtVocab, PhraseVocab &tgtPhraseVoc);

//exists for hiero rules
bool existsRule(MosesTraining::PHRASE_ID src, MosesTraining::PHRASE_ID tgt, PhraseTranslations &transTable);

#endif
