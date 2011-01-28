#include "TerScorer.h"
#include "TERsrc/tercalc.h"
#include "TERsrc/terAlignment.h"

const int TerScorer::LENGTH = 4;
using namespace TERCpp;
using namespace std;


void TerScorer::setReferenceFiles ( const vector<string>& referenceFiles )
{
        // for each line in the reference file, create a multiset of the
        // word ids
//         if ( referenceFiles.size() != 1 )
//         {
//                 throw runtime_error ( "TER only supports a single reference" );
//         }
        for ( int incRefs = 0; incRefs < ( int ) referenceFiles.size(); incRefs++ )
        {

                stringstream convert;
                m_references.clear();
//                 cerr << "taille de  " << ( int ) m_references.size() << endl;
//                 convert << getenv ( "JAVA" );
//                 javaEnv = convert.str();
//                 convert.str ( "" );
//                 convert << getenv ( "TERCOM" );
//                 tercomEnv = convert.str();
//                 convert.str ( "" );
//                 convert << getpid();
//                 m_pid = convert.str();
//                 convert.str ( "" );
//     if (((int)tercomEnv.size()==0) || ((int)javaEnv.size()==0) )
//     {
//         stringstream msg;
//         msg << "TERCOM or JAVA environment variable are not set, please check theses commands : "<<endl<<"echo \"Path to tercom : $TERCOM\" ; echo \"Path to java : $JAVA\" ; ";
//         throw runtime_error(msg.str());
//         exit(0);
//     }
                _reftokens.clear();
                _reflengths.clear();
                ifstream in ( referenceFiles.at ( incRefs ).c_str() );
                if ( !in )
                {
                        throw runtime_error ( "Unable to open " + referenceFiles.at ( incRefs ) );
                }
                string line;
                int sid = 0;
                while ( getline ( in, line ) )
                {
                        vector<int> tokens;
                        encode ( line, tokens );
                        m_references.push_back ( tokens );
//         _reftokens.push_back(multiset<int>());
//         for (size_t i = 0; i < tokens.size(); ++i) {
//             _reftokens.back().insert(tokens[i]);
//         }
//         _reflengths.push_back(tokens.size());
//         if (sid > 0 && sid % 100 == 0) {
                        TRACE_ERR ( "." );
//         }
                        ++sid;
                }
                m_multi_references.push_back ( m_references );
        }
        TRACE_ERR ( endl );
}

void TerScorer::prepareStats ( size_t sid, const string& text, ScoreStats& entry )
{
//     cerr << "test de "<<sid<<endl;
        if ( sid >= m_references.size() )
        {
                stringstream msg;
                msg << "Sentence id (" << sid << ") not found in reference set";
                throw runtime_error ( msg.str() );
        }

        terAlignment result;
        for ( int incRefs = 0; incRefs < ( int ) m_multi_references.size(); incRefs++ )
        {
                vector<int> testtokens;
                vector<int> reftokens;
                reftokens = m_multi_references.at ( incRefs ).at ( sid );
                encode ( text, testtokens );
                terCalc evaluation;
                evaluation.setDebugMode ( false );
                terAlignment tmp_result = evaluation.TER ( reftokens, testtokens );
                if ( ( result.numEdits == 0.0 ) && ( result.numWords == 0.0 ) )
                {
                        result = tmp_result;
                }
                else
                        if ( result.score() > tmp_result.score() )
                        {
                                result = tmp_result;
                        }
        }

        ostringstream stats;
        stats << result.numEdits << " " << result.numWords << " " << result.score() << " " ;
//     stats << correct << " " << testtokens.size() << " " << _reflengths[sid] << " " ;
//     stats << l_return;
        string stats_str = stats.str();
//     cerr << "TER RETURNS : " + stats_str << endl;
        entry.set ( stats_str );
}

float TerScorer::calculateScore ( const vector<int>& comps )
{
        cerr << "TerScorer::calculateScore called" << endl;
        float denom = 1.0 * comps[1];
        float num =  -1.0 * comps[0];
        if ( denom == 0 )
        {
//         shouldn't happen!
                cerr << "CalculateScore Gives : " << num << "/" << denom << "=0.0" << endl;
                return 0.0;
        }
        else
        {
                cerr << "CalculateScore Gives : " << num << "/" << denom << "=" << num / denom << endl;
                return num / denom;
        }
}
float TerScorer::calculateScore ( const vector<float>& comps )
{
        cerr << "TerScorer::calculateScore called" << endl;
        float denom = 1.0 * comps[1];
        float num =  -1.0 * comps[0];
        if ( denom == 0 )
        {
//         shouldn't happen!
                cerr << "CalculateScore Gives : " << num << "/" << denom << "=0.0" << endl;
                return 0.0;
        }
        else
        {
                cerr << "CalculateScore Gives : " << num << "/" << denom << "=" << num / denom << endl;
                return num / denom;
        }
}
