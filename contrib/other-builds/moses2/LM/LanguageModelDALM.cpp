/*
 * LanguageModelDALM.cpp
 *
 *  Created on: 5 Dec 2015
 *      Author: hieu
 */

#include "LanguageModelDALM.h"
#include "dalm.h"
#include "util/exception.hh"

using namespace std;

/////////////////////////
void read_ini(const char *inifile, string &model, string &words, string &wordstxt)
{
  ifstream ifs(inifile);
  string line;

  getline(ifs, line);
  while(ifs) {
    unsigned int pos = line.find("=");
    string key = line.substr(0, pos);
    string value = line.substr(pos+1, line.size()-pos);
    if(key=="MODEL") {
      model = value;
    } else if(key=="WORDS") {
      words = value;
    } else if(key=="WORDSTXT") {
      wordstxt = value;
    }
    getline(ifs, line);
  }
}
/////////////////////////

LanguageModelDALM::LanguageModelDALM(size_t startInd, const std::string &line)
:StatefulFeatureFunction(startInd, line)
{
	ReadParameters();
}

LanguageModelDALM::~LanguageModelDALM() {
	// TODO Auto-generated destructor stub
}

void LanguageModelDALM::Load(System &system)
{
  /////////////////////
  // READING INIFILE //
  /////////////////////
  string inifile= m_filePath + "/dalm.ini";

  string model; // Path to the double-array file.
  string words; // Path to the vocabulary file.
  string wordstxt; //Path to the vocabulary file in text format.
  read_ini(inifile.c_str(), model, words, wordstxt);

  model = m_filePath + "/" + model;
  words = m_filePath + "/" + words;
  wordstxt = m_filePath + "/" + wordstxt;

  UTIL_THROW_IF(model.empty() || words.empty() || wordstxt.empty(),
                util::FileOpenException,
                "Failed to read DALM ini file " << m_filePath << ". Probably doesn't exist");

  ////////////////
  // LOADING LM //
  ////////////////

  // Preparing a logger object.
  m_logger = new DALM::Logger(stderr);
  m_logger->setLevel(DALM::LOGGER_INFO);

  // Load the vocabulary file.
  m_vocab = new DALM::Vocabulary(words, *m_logger);

  // Load the language model.
  m_lm = new DALM::LM(model, *m_vocab, m_nGramOrder, *m_logger);

  //wid_start = m_vocab->lookup(BOS_);
  //wid_end = m_vocab->lookup(EOS_);

  // vocab mapping
  CreateVocabMapping(wordstxt);

}

void LanguageModelDALM::CreateVocabMapping(const std::string &wordstxt)
{

}

void LanguageModelDALM::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "factor") {
	m_factorType = Scan<FactorType>(value);
  } else if (key == "order") {
	m_nGramOrder = Scan<size_t>(value);
  } else if (key == "path") {
	m_filePath = value;
  } else {
	  StatefulFeatureFunction::SetParameter(key, value);
  }
  m_ContextSize = m_nGramOrder-1;
}

FFState* LanguageModelDALM::BlankState(const Manager &mgr, const PhraseImpl &input) const
{

}

void LanguageModelDALM::EmptyHypothesisState(FFState &state, const Manager &mgr, const PhraseImpl &input) const
{

}

 void LanguageModelDALM::EvaluateInIsolation(const System &system,
		  const Phrase &source, const TargetPhrase &targetPhrase,
          Scores &scores,
          Scores *estimatedScores) const
 {

 }

 void LanguageModelDALM::EvaluateWhenApplied(const Manager &mgr,
    const Hypothesis &hypo,
    const FFState &prevState,
    Scores &scores,
	FFState &state) const
 {

 }

