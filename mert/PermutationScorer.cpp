#include <cassert>
#include "PermutationScorer.h"

using namespace std;

namespace MosesTuning
{


const int PermutationScorer::SCORE_PRECISION = 5;
const int PermutationScorer::SCORE_MULTFACT = 100000; // 100000=10^SCORE_PRECISION

PermutationScorer::PermutationScorer(const string &distanceMetric, const string &config)
  :StatisticsBasedScorer(distanceMetric,config)
{
  //configure regularisation

  static string KEY_REFCHOICE = "refchoice";
  static string REFCHOICE_AVERAGE = "average";
  static string REFCHOICE_CLOSEST = "closest";

  string refchoice = getConfig(KEY_REFCHOICE,REFCHOICE_CLOSEST);
  if (refchoice == REFCHOICE_AVERAGE) {
    m_refChoiceStrategy = REFERENCE_CHOICE_AVERAGE;
  } else if (refchoice == REFCHOICE_CLOSEST) {
    m_refChoiceStrategy = REFERENCE_CHOICE_CLOSEST;
  } else {
    throw runtime_error("Unknown reference choice strategy: " + refchoice);
  }
  cerr << "Using reference choice strategy: " << refchoice << endl;

  if (distanceMetric.compare("HAMMING") == 0) {
    m_distanceMetric = HAMMING_DISTANCE;
  } else if (distanceMetric.compare("KENDALL") == 0) {
    m_distanceMetric = KENDALL_DISTANCE;
  }
  cerr << "Using permutation distance metric: " << distanceMetric << endl;

  //Get reference alignments from scconfig refalign option
  static string KEY_ALIGNMENT_FILES = "refalign";
  string refalign = getConfig(KEY_ALIGNMENT_FILES,"");
  //cout << refalign << endl;
  if (refalign.length() > 0) {
    string substring;
    while (!refalign.empty()) {
      getNextPound(refalign, substring, "+");
      m_referenceAlignments.push_back(substring);
    }
  }

  //Get length of source sentences read in from scconfig source option
  // this is essential for extractor but unneccesary for mert executable
  static string KEY_SOURCE_FILE = "source";
  string sourceFile = getConfig(KEY_SOURCE_FILE,"");
  if (sourceFile.length() > 0) {
    cerr << "Loading source sentence lengths from " << sourceFile << endl;
    ifstream sourcein(sourceFile.c_str());
    if (!sourcein) {
      throw runtime_error("Unable to open: " + sourceFile);
    }
    string line;
    while (getline(sourcein,line)) {
      size_t wordNumber = 0;
      string word;
      while(!line.empty()) {
        getNextPound(line, word, " ");
        wordNumber++;
      }
      m_sourceLengths.push_back(wordNumber);
    }
    sourcein.close();
  }
}

void PermutationScorer::setReferenceFiles(const vector<string>& referenceFiles)
{
  cout << "*******setReferenceFiles" << endl;
  //make sure reference data is clear
  m_referencePerms.clear();

  vector< vector< int> > targetLengths;
  //Just getting target length from reference text file
  for (size_t i = 0; i < referenceFiles.size(); ++i) {
    vector <int> lengths;
    cout << "Loading reference from " << referenceFiles[i] << endl;
    ifstream refin(referenceFiles[i].c_str());
    if (!refin) {
      cerr << "Unable to open: " << referenceFiles[i] << endl;
      throw runtime_error("Unable to open alignment file");
    }
    string line;
    while (getline(refin,line)) {
      int count = getNumberWords(line);
      lengths.push_back(count);
    }
    targetLengths.push_back(lengths);
  }

  //load reference data
  //NOTE ignoring normal reference file, only using previously saved alignment reference files
  for (size_t i = 0; i < m_referenceAlignments.size(); ++i) {
    vector<Permutation> referencePerms;
    cout << "Loading reference from " << m_referenceAlignments[i] << endl;
    ifstream refin(m_referenceAlignments[i].c_str());
    if (!refin) {
      cerr << "Unable to open: " << m_referenceAlignments[i] << endl;
      throw runtime_error("Unable to open alignment file");
    }
    string line;
    size_t sid = 0; //sentence counter
    while (getline(refin,line)) {
      //cout << line << endl;

      //Line needs to be of the format: 0-0 1-1 1-2 etc source-target
      Permutation perm(line, m_sourceLengths[sid],targetLengths[i][sid]);
      //perm.dump();
      referencePerms.push_back(perm);
      //check the source sentence length is the same for previous file
      if (perm.getLength() !=  m_sourceLengths[sid]) {
        cerr << "Permutation Length: " << perm.getLength() << endl;
        cerr << "Source length: " << m_sourceLengths[sid] << " for sid " << sid << endl;
        throw runtime_error("Source sentence lengths not the same: ");
      }

      sid++;
    }
    m_referencePerms.push_back(referencePerms);
  }
}

int PermutationScorer::getNumberWords (const string& text) const
{
  int count = 0;
  string line = trimStr(text);
  if (line.length()>0) {
    int pos = line.find(" ");
    while (pos!=int(string::npos)) {
      count++;
      pos = line.find(" ",pos+1);
    }
    count++;
  }
  return count;
}


void PermutationScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry)
{
  //bool debug= (verboselevel()>3); // TODO: fix verboselevel()
  bool debug=false;
  if (debug) {
    cout << "*******prepareStats" ;
    cout << text << endl;
    cout << sid << endl;
    cout << "Reference0align:" << endl;
    m_referencePerms[0][sid].dump();
  }

  string sentence = "";
  string align = text;
  size_t alignmentData = text.find("|||");
  //Get sentence and alignment parts
  if(alignmentData != string::npos) {
    getNextPound(align,sentence, "|||");
  } else {
    align = text;
  }
  int translationLength = getNumberWords(sentence);


  //A vector of Permutations for each sentence
  vector< vector<Permutation> > nBestPerms;
  float distanceValue;

  //need to create permutations for each nbest line
  //here we check if the alignments extracted from the nbest are phrase-based or word-based, in which case no conversion is needed
  bool isWordAlignment=true;
  string alignCopy = align;
  string align1;
  getNextPound(alignCopy,align1," ");
  if (align1.length() > 0) {
    size_t phraseDelimeter = align1.find("=");
    if(phraseDelimeter!= string::npos)
      isWordAlignment=false;
  }
  string standardFormat = align;
  if(!isWordAlignment)
    standardFormat= Permutation::convertMosesToStandard(align);

  if (debug) {
    cerr << "Nbest alignment:  " << align << endl;
    cerr << "-->std alignment: " << standardFormat << endl;
  }

  Permutation perm(standardFormat, m_sourceLengths[sid],translationLength);
  //perm.dump();

  if (m_refChoiceStrategy == REFERENCE_CHOICE_AVERAGE) {
    float total = 0;
    for (size_t i = 0; i < m_referencePerms.size(); ++i) {
      float dist = perm.distance(m_referencePerms[i][sid], m_distanceMetric);
      total += dist;
      //cout << "Ref number: " << i << " distance: " << dist << endl;
    }
    float mean = (float)total/m_referencePerms.size();
    //cout << "MultRef strategy AVERAGE: total " << total << " mean " << mean << " number " << m_referencePerms.size() << endl;
    distanceValue = mean;
  } else if (m_refChoiceStrategy == REFERENCE_CHOICE_CLOSEST)  {
    float max_val = 0;

    for (size_t i = 0; i < m_referencePerms.size(); ++i) {
      //look for the closest reference
      float value = perm.distance(m_referencePerms[i][sid], m_distanceMetric);
      //cout << "Ref number: " << i << " distance: " << value << endl;
      if (value > max_val) {
        max_val = value;
      }
    }
    distanceValue = max_val;
    //cout << "MultRef strategy CLOSEST: max_val " << distanceValue << endl;
  } else {
    throw runtime_error("Unsupported reflength strategy");
  }

  //SCOREROUT eg: 0.04546
  distanceValue*=SCORE_MULTFACT; //SCOREROUT eg: 4546 to transform float into integer
  ostringstream tempStream;
  tempStream.precision(0);	// decimal precision not needed as score was multiplied per SCORE_MULTFACT
  tempStream << std::fixed << distanceValue << " 1"; //use for final normalization over the amount of test sentences
  string str = tempStream.str();
  entry.set(str);

//cout << distanceValue << "=" << distanceValue << " (str:" << tempStream.str() << ")" << endl;
}

//Will just be final score
statscore_t PermutationScorer::calculateScore(const vector<ScoreStatsType>& comps) const
{
  //cerr << "*******PermutationScorer::calculateScore" ;
  //cerr << " " << comps[0]/comps[1] << endl;
  return (((statscore_t) comps[0]) / comps[1]) / SCORE_MULTFACT;
}

}

