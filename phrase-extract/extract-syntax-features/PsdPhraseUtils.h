#ifndef psd_phrase_utils_h
#define psd_phrase_utils_h
#include "tables-core.h"
#include <set>
#include <map>

#define LINE_MAX_LENGTH 10000

//DAMT HIERO NOTE : If used as interface for feature extraction code always use methods with Rule for hiero

using namespace std;

typedef MosesTraining::PhraseTable PhraseVocab;
typedef multimap< MosesTraining::PHRASE_ID, MosesTraining::PHRASE_ID > PhraseTranslations;

bool readPhraseVocab(const char* vocabFile, MosesTraining::Vocabulary &wordVocab, PhraseVocab &vocab);

// Read Vocbulary for hiero rules
bool readRuleVocab(const char* vocabFile, MosesTraining::Vocabulary &wordVocab, PhraseVocab &vocab);

bool readPhraseTranslations(const char *ptFile, MosesTraining::Vocabulary &srcWordVocab, MosesTraining::Vocabulary &tgtWordVocab, PhraseVocab &srcPhraseVocab, PhraseVocab &tgtPhraseVocab, PhraseTranslations &transTable);

bool readPhraseTranslations(const char *ptFile, MosesTraining::Vocabulary &srcWordVocab, MosesTraining::Vocabulary &tgtWordVocab, PhraseVocab &srcPhraseVocab, PhraseVocab &tgtPhraseVocab, PhraseTranslations &transTable, map<string,string> &transTableScores);

//Read rule table
bool readRules(const char *ptFile, MosesTraining::Vocabulary &srcWordVocab, MosesTraining::Vocabulary &tgtWordVocab, PhraseVocab &srcPhraseVocab, PhraseVocab &tgtPhraseVocab, PhraseTranslations &transTable);

MosesTraining::PHRASE makePhrase(const string phrase, MosesTraining::Vocabulary &wordVocab);

MosesTraining::PHRASE makePhraseAndVoc(const string phrase, MosesTraining::Vocabulary &wordVocab);

//Make Vocabulary for hiero rules
MosesTraining::PHRASE makeRuleAndVoc(const string rule, MosesTraining::Vocabulary &wordVocab);

MosesTraining::PHRASE_ID getPhraseID(const string phrase, MosesTraining::Vocabulary &wordVocab, PhraseVocab &vocab);

//Get ID of hiero rules
MosesTraining::PHRASE_ID getRuleID(const string rule, MosesTraining::Vocabulary &wordVocab, PhraseVocab &vocab);

string getPhrase(MosesTraining::PHRASE_ID labelid, MosesTraining::Vocabulary &tgtVocab, PhraseVocab &tgtPhraseVoc);

//get target hiero rule, equivalent of get phrase. If use of
string getTargetRule(MosesTraining::PHRASE_ID labelid, MosesTraining::Vocabulary &tgtVocab, PhraseVocab &tgtPhraseVoc);

//exists for hiero rules
bool existsRule(MosesTraining::PHRASE_ID src, MosesTraining::PHRASE_ID tgt, PhraseTranslations &transTable);

bool exists(MosesTraining::PHRASE_ID src, MosesTraining::PHRASE_ID tgt, PhraseTranslations &transTable);

bool exists(MosesTraining::PHRASE_ID src, PhraseTranslations &transTable);

#endif
