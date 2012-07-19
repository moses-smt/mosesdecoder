#ifndef psd_phrase_utils_h
#define psd_phrase_utils_h
#include "tables-core.h"
#include "FeatureExtractor.h"
#include <set>
#include <map>

#define LINE_MAX_LENGTH 10000


using namespace std;

typedef MosesTraining::PhraseTable PhraseVocab;
typedef multimap< MosesTraining::PHRASE_ID, PSD::Translation > PhraseTranslations;

bool readPhraseVocab(const char* vocabFile, MosesTraining::Vocabulary &wordVocab, PhraseVocab &vocab);

bool readPhraseTranslations(const char *ptFile, MosesTraining::Vocabulary &srcWordVocab, MosesTraining::Vocabulary &tgtWordVocab, PhraseVocab &srcPhraseVocab, PhraseVocab &tgtPhraseVocab, PhraseTranslations &transTable);

MosesTraining::PHRASE makePhrase(const string &phrase, MosesTraining::Vocabulary &wordVocab);
MosesTraining::PHRASE makePhraseAndVoc(const string &phrase, MosesTraining::Vocabulary &wordVocab);

MosesTraining::PHRASE_ID getPhraseID(const string &phrase, MosesTraining::Vocabulary &wordVocab, PhraseVocab &vocab);

string getPhrase(MosesTraining::PHRASE_ID labelid, MosesTraining::Vocabulary &tgtVocab, PhraseVocab &tgtPhraseVoc);

bool exists(MosesTraining::PHRASE_ID src, MosesTraining::PHRASE_ID tgt, PhraseTranslations &transTable);

bool exists(MosesTraining::PHRASE_ID src, PhraseTranslations &transTable);

#endif
