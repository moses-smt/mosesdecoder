#include "TerScorer.h"
#include "TERsrc/tercalc.h"
#include "TERsrc/terAlignment.h"

const int TerScorer::LENGTH = 4;
using namespace TERCpp;
using namespace std;


void TerScorer::setReferenceFiles(const vector<string>& referenceFiles)
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
//     if (((int)tercomEnv.size()==0) || ((int)javaEnv.size()==0) )
//     {
//         stringstream msg;
//         msg << "TERCOM or JAVA environment variable are not set, please check theses commands : "<<endl<<"echo \"Path to tercom : $TERCOM\" ; echo \"Path to java : $JAVA\" ; ";
//         throw runtime_error(msg.str());
//         exit(0);
//     }
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
//         _reftokens.push_back(multiset<int>());
//         for (size_t i = 0; i < tokens.size(); ++i) {
//             _reftokens.back().insert(tokens[i]);
//         }
//         _reflengths.push_back(tokens.size());
//         if (sid > 0 && sid % 100 == 0) {
        TRACE_ERR(".");
//         }
        ++sid;
    }
    TRACE_ERR(endl);
}

void TerScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry)
{
//     cerr << "test de "<<sid<<endl;
    if (sid >= m_references.size()) {
        stringstream msg;
        msg << "Sentence id (" << sid << ") not found in reference set";
        throw runtime_error(msg.str());
    }
//     string javaEnv;
//     string tercomEnv;
//     javaEnv=getenv("JAVA");
//     char * javaEnvChar;
    //take back the environment variables for JAVA and TERCOM
//     cerr << "Environnement JAVA : "<< javaEnvChar <<endl;
//     copy(*javaEnvChar[0],*javaEnvChar[sizeof(javaEnvChar)],javaEnv);
//     cerr << "Environnement JAVA : "<< javaEnv <<endl;
//     cerr << "Environnement TERCOM : "<< tercomEnv <<endl;
//     cerr <<endl;
//     stringstream l_test;
//     l_test.str("");
//     stringstream l_ref;
//     l_ref.str("");
    //calculate correct, output_length and ref_length for
    //the line and store it in entry
    vector<int> testtokens;
    vector<int> reftokens;
    reftokens=m_references.at(sid);
    encode(text,testtokens);
    terCalc evaluation;
    evaluation.setDebugMode ( false );
    terAlignment result=evaluation.TER(reftokens,testtokens);
    
    
    
//     m_test=testtokens;
//     cerr << "PID : " << getpid()<<endl;
//     cerr << endl;


//     ofstream testFile(("/var/tmp/hyp.TERScorer."+ m_pid +".txt").c_str());
//     if (testFile)
//     {
//         for (vector<int>::iterator it=testtokens.begin(); it!=testtokens.end(); it++)
//         {
//             testFile << (*it) << " ";
//         }
//         testFile << "(ID"<<sid << ")"<<endl;
//         testFile.close();
//     }
//     else
//     {
//         stringstream msg;
//         msg << "Error TerScorer::prepareStats : hyp-writing" << endl;
//         throw runtime_error(msg.str());
//     }
//     ofstream refFile(("/var/tmp/ref.TERScorer."+ m_pid +".txt").c_str());
//     if (refFile)
//     {
//         for (vector<int>::iterator it=testtokens.begin(); it!=testtokens.end(); it++)
//         {
//             refFile << (*it) << " ";
//         }
//         refFile << "(ID"<<sid << ")"<<endl;
//         refFile.close();
//     }
//     else
//     {
//         stringstream msg;
//         msg << "Error TerScorer::prepareStats : ref-writing" << endl;
//         throw runtime_error(msg.str());
//     }

//     string commande("java -Dfile.encoding=utf8 -jar tercom.7.25.jar -r /var/tmp/ref.TERScorer.txt -h /var/tmp/hyp.TERScorer.txt -o ter -n /var/tmp/out.TERScorer.txt");
//     string commande("");
//     commande+=javaEnv+" -jar " + tercomEnv +"  -r /var/tmp/ref.TERScorer."+ m_pid +".txt -h /var/tmp/hyp.TERScorer."+ m_pid +".txt -o ter -n /var/tmp/out.TERScorer."+ m_pid +".txt";
//     ("/usr/bin/java -jar /home/servan/Programmations/tercom-0.7.25/tercom.7.25.jar -r /var/tmp/ref.TERScorer.txt -h /var/tmp/hyp.TERScorer.txt -o ter -n /var/tmp/out.TERScorer.txt");
//     multiset<int> testtokens_all(testtokens.begin(),testtokens.end());
//     set<int> testtokens_unique(testtokens.begin(),testtokens.end());
//     commande += " -b " + beamWidth;
//     commande += " -d " + maxShiftDist;
//     if (caseSensitive) { commande += " -S"; }
//     if (withPunctuation) { commande += " -P"; }
//     system(commande.c_str());
//     ifstream resultFile(("/var/tmp/out.TERScorer."+ m_pid +".txt.ter").c_str());
//     string resultLine;
//     string l_return;
//     bool pushBack=false;
//     if (resultFile.is_open())
//     {
        //two calls in order to skip the hyp and ref filename in resultFile
//         getline(resultFile,resultLine);
//         getline(resultFile,resultLine);
//         getline(resultFile,resultLine);
//         for (int i=0; i<(int)resultLine.length();i++)
//         {
// 	    if ((resultLine[i]=='.') && (i>0))
// 	    {
// 	      pushBack=false;
// 	    }
//             if (pushBack==true)
//             {
//                 l_return.push_back(resultLine[i]);
//             }
//             if ((resultLine[i]==' ') && (i>0))
//             {
//                 pushBack=true;
//             }
//         }
//     }
//     else
//     {
//         stringstream msg;
//         msg << "Error TerScorer::prepareStats : result-reading" << endl;
//         throw runtime_error(msg.str());
//     }

//     int correct = 0;
//     for (set<int>::iterator i = testtokens_unique.begin();
//             i != testtokens_unique.end(); ++i) {
//         int token = *i;
//         correct += min(_reftokens[sid].count(token), testtokens_all.count(token));
//     }
//     if (remove(("/var/tmp/out.TERScorer."+ m_pid +".txt.ter").c_str())!=0)
//     {
//         stringstream msg;
//         msg << "Error TerScorer::prepareStats : result-deleting" << endl;
//         throw runtime_error(msg.str());
//     }
//     if (remove(("/var/tmp/hyp.TERScorer."+ m_pid +".txt").c_str())!=0)
//     {
//         stringstream msg;
//         msg << "Error TerScorer::prepareStats : hyp-deleting" << endl;
//         throw runtime_error(msg.str());
//     }
//     if (remove(("/var/tmp/ref.TERScorer."+ m_pid +".txt").c_str())!=0)
//     {
//         stringstream msg;
//         msg << "Error TerScorer::prepareStats : ref-deleting" << endl;
//         throw runtime_error(msg.str());
//     }

    ostringstream stats;
     stats << result.numEdits<< " " << result.numWords << " " <<result.score()<< " " ;
//     stats << correct << " " << testtokens.size() << " " << _reflengths[sid] << " " ;
//     stats << l_return;
    string stats_str = stats.str();
//     cerr << "TER RETURNS : " + stats_str << endl;
    entry.set(stats_str);
}

float TerScorer::calculateScore(const vector<int>& comps)
{
    cerr<< "TerScorer::calculateScore called" <<endl;
    float denom = 1.0*comps[1];
    float num =  1.0*comps[0];
    if (denom == 0) {
//         shouldn't happen!
        cerr << "CalculateScore Gives : " << num <<"/" <<denom << "=0.0" <<endl;
        return 0.0;
    } else {
        cerr << "CalculateScore Gives : " << num <<"/" <<denom << "=" << num/denom<<endl;
        return num/denom;
    }
}
float TerScorer::calculateScore(const vector<float>& comps)
{
    cerr<< "TerScorer::calculateScore called" <<endl;
    float denom = 1.0*comps[1];
    float num =  1.0*comps[0];
    if (denom == 0) {
//         shouldn't happen!
        cerr << "CalculateScore Gives : " << num <<"/" <<denom << "=0.0" <<endl;
        return 0.0;
    } else {
        cerr << "CalculateScore Gives : " << num <<"/" <<denom << "=" << num/denom<<endl;
        return num/denom;
    }
}
