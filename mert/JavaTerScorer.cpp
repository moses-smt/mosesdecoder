#include "JavaTerScorer.h"

const int JavaTerScorer::LENGTH = 4;


void JavaTerScorer::setReferenceFiles(const vector<string>& referenceFiles)
{
  // for each line in the reference file, create a multiset of the
  // word ids
  if (referenceFiles.size() != 1) {
    throw runtime_error("TER only supports a single reference");
  }
  stringstream convert;
  cerr << "taille de  "<<(int)m_references.size()<<endl;
  convert << getenv("JAVA");
  javaEnv=convert.str();
  convert.str("");
  convert << getenv("TERCOM");
  tercomEnv=convert.str();
  convert.str("");
  convert << getpid();
  m_pid=convert.str();
  convert.str("");
  if (((int)tercomEnv.size()==0) || ((int)javaEnv.size()==0) ) {
    stringstream msg;
    msg << "TERCOM or JAVA environment variable are not set, please check theses commands : "<<endl<<"echo \"Path to tercom : $TERCOM\" ; echo \"Path to java : $JAVA\" ; ";
    throw runtime_error(msg.str());
    exit(0);
  }
  _reftokens.clear();
  _reflengths.clear();
  ifstream in(referenceFiles[0].c_str());
  if (!in) {
    throw runtime_error("Unable to open " + referenceFiles[0]);
  }
  string line;
  int sid = 0;
  while (getline(in,line)) {
    vector<int> tokens;
    encode(line,tokens);
    m_references.push_back(tokens);
    TRACE_ERR(".");
    ++sid;
  }
  TRACE_ERR(endl);
}

void JavaTerScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry)
{
  cerr << "test de "<<sid<<endl;
  if (sid >= m_references.size()) {
    stringstream msg;
    msg << "Sentence id (" << sid << ") not found in reference set";
    throw runtime_error(msg.str());
  }
  vector<int> testtokens;
  vector<int> reftokens;
  reftokens=m_references.at(sid);
  encode(text,testtokens);


  ofstream testFile(("/var/tmp/hyp.JAVATERScorer."+ m_pid +".txt").c_str());
  if (testFile) {
    for (vector<int>::iterator it=testtokens.begin(); it!=testtokens.end(); it++) {
      testFile << (*it) << " ";
    }
    testFile << "(ID"<<sid << ")"<<endl;
    testFile.close();
  } else {
    stringstream msg;
    msg << "Error JavaTerScorer::prepareStats : hyp-writing" << endl;
    throw runtime_error(msg.str());
  }
  ofstream refFile(("/var/tmp/ref.JAVATERScorer."+ m_pid +".txt").c_str());
  if (refFile) {
    for (vector<int>::iterator it=reftokens.begin(); it!=reftokens.end(); it++) {
      refFile << (*it) << " ";
    }
    refFile << "(ID"<<sid << ")"<<endl;
    refFile.close();
  } else {
    stringstream msg;
    msg << "Error JavaTerScorer::prepareStats : ref-writing" << endl;
    throw runtime_error(msg.str());
  }

  string commande("");
  commande+=javaEnv+" -jar " + tercomEnv +"  -r /var/tmp/ref.JAVATERScorer."+ m_pid +".txt -h /var/tmp/hyp.JAVATERScorer."+ m_pid +".txt -o ter -n /var/tmp/out.JAVATERScorer."+ m_pid +".txt";
  system(commande.c_str());
  ifstream resultFile(("/var/tmp/out.JAVATERScorer."+ m_pid +".txt.ter").c_str());
  string resultLine;
  string l_return;
  bool pushBack=false;
  if (resultFile.is_open()) {
    //two calls in order to skip the hyp and ref filename in resultFile
    getline(resultFile,resultLine);
    getline(resultFile,resultLine);
    getline(resultFile,resultLine);
    for (int i=0; i<(int)resultLine.length(); i++) {
      if (pushBack==true) {
        l_return.push_back(resultLine[i]);
      }
      if ((resultLine[i]==' ') && (i>0)) {
        pushBack=true;
      }
    }
  } else {
    stringstream msg;
    msg << "Error JavaTerScorer::prepareStats : result-reading" << endl;
    throw runtime_error(msg.str());
  }

  ostringstream stats;
  stats << l_return;
  string stats_str = stats.str();
  cerr << "JAVATER RETURNS : " + stats_str << endl;
  entry.set(stats_str);
}

float JavaTerScorer::calculateScore(const vector<int>& comps)
{
  cerr<< "JavaTerScorer::calculateScore called" <<endl;
  float denom = comps[1];
  float num = comps[0];
  if (denom == 0) {
//         shouldn't happen!
    cerr << "CalculateScore Gives : " << num <<"/" <<denom << "=1.0" <<endl;
    return 1.0;
  } else {
    cerr << "CalculateScore Gives : " << num <<"/" <<denom << "=" << 1.0-(num/denom)<<endl;
    return 1.0-(num/denom);
  }
}

float JavaTerScorer::calculateScore(const vector<float>& comps)
{
  cerr<< "JavaTerScorer::calculateScore called" <<endl;
  float denom = comps[1];
  float num = comps[0];
  if (denom == 0) {
//         shouldn't happen!
    cerr << "CalculateScore Gives : " << num <<"/" <<denom << "=1.0" <<endl;
    return 1.0;
  } else {
    cerr << "CalculateScore Gives : " << num <<"/" <<denom << "=" << 1.0-(num/denom)<<endl;
    return 1.0-(num/denom);
  }
}
