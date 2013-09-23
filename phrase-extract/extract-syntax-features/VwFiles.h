#ifndef __VW_FILES_HPP_
#define __VW_FILES_HPP_


#include <vector>
#include <string>
#include "PsdPhraseUtils.h"

//#include <map>
//#include "ContextFeatureSet.h"

using namespace std;
using namespace MosesTraining;

string makeVwTestingInstance(vector<string>& context);

string makeVwTrainingInstance(vector<string>& context,int label);


string makeVwLine(int label, float cost, string namesp, vector<string>& label_features);
/*
string makeVwSrcFeatures();

string makeVwPairFeatures(int label, vector<string>& pFeats);

string makeVwLabelFeatures(int label, vector<string>& labelFeats);

*/
string makeVwGlobalTrainingInstance(PHRASE_ID src, vector<string>& context,set<PHRASE_ID> labels, PhraseTranslations &transTable, PhraseVocab &pVocab, Vocabulary &wVocab);

string makeVwGlobalTestingInstance(PHRASE_ID src, vector<string>& context, PhraseTranslations &transTable, PhraseVocab &pVocab, Vocabulary &wVocab);

string makeVwHeaderLine(int label, vector<string>& label_features);

bool printVwHeaderFile(string fileName,
		       PhraseTranslations &transTable,
		       PHRASE_ID src,
		       PhraseVocab &pVocab,
		       Vocabulary &wVocab);

bool printVwHeaderFile(string fileName,
		       PhraseTranslations &transTable,
		       PhraseVocab &pVocab,
		       Vocabulary &wVocab);

string escapeVwSpecialChars(string input);

#endif
