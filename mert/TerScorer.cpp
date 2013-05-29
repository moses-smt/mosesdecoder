#include "TerScorer.h"

#include <cmath>
#include <sstream>
#include <stdexcept>

#include "ScoreStats.h"
#include "TER/tercalc.h"
#include "TER/terAlignment.h"
#include "Util.h"

using namespace std;
using namespace TERCpp;

namespace MosesTuning
{


TerScorer::TerScorer(const string& config)
  : StatisticsBasedScorer("TER",config), kLENGTH(2) {}

TerScorer::~TerScorer() {}

void TerScorer::setReferenceFiles ( const vector<string>& referenceFiles )
{
  // for each line in the reference file, create a multiset of the
  // word ids.
  for ( int incRefs = 0; incRefs < ( int ) referenceFiles.size(); incRefs++ ) {
    stringstream convert;
    m_references.clear();

    m_ref_tokens.clear();
    m_ref_lengths.clear();
    ifstream in ( referenceFiles.at ( incRefs ).c_str() );
    if ( !in ) {
      throw runtime_error ( "Unable to open " + referenceFiles.at ( incRefs ) );
    }
    string line;
    int sid = 0;
    while ( getline ( in, line ) ) {
      line = this->preprocessSentence(line);
      vector<int> tokens;
      TokenizeAndEncode(line, tokens);
      m_references.push_back ( tokens );
      TRACE_ERR ( "." );
      ++sid;
    }
    m_multi_references.push_back ( m_references );
  }

  TRACE_ERR ( endl );
  m_references=m_multi_references.at(0);
}

void TerScorer::prepareStats ( size_t sid, const string& text, ScoreStats& entry )
{
  string sentence = this->preprocessSentence(text);

  terAlignment result;
  result.numEdits = 0.0 ;
  result.numWords = 0.0 ;
  result.averageWords = 0.0;

  for ( int incRefs = 0; incRefs < ( int ) m_multi_references.size(); incRefs++ ) {
    if ( sid >= m_multi_references.at(incRefs).size() ) {
      stringstream msg;
      msg << "Sentence id (" << sid << ") not found in reference set";
      throw runtime_error ( msg.str() );
    }

    vector<int> testtokens;
    vector<int> reftokens;
    reftokens = m_multi_references.at ( incRefs ).at ( sid );
    double averageLength=0.0;
    for ( int incRefsBis = 0; incRefsBis < ( int ) m_multi_references.size(); incRefsBis++ ) {
      if ( sid >= m_multi_references.at(incRefsBis).size() ) {
        stringstream msg;
        msg << "Sentence id (" << sid << ") not found in reference set";
        throw runtime_error ( msg.str() );
      }
      averageLength+=(double)m_multi_references.at ( incRefsBis ).at ( sid ).size();
    }
    averageLength=averageLength/( double ) m_multi_references.size();
    TokenizeAndEncode(sentence, testtokens);
    terCalc * evaluation=new terCalc();
    evaluation->setDebugMode ( false );
    terAlignment tmp_result = evaluation->TER ( reftokens, testtokens );
    tmp_result.averageWords=averageLength;
    if ( ( result.numEdits == 0.0 ) && ( result.averageWords == 0.0 ) ) {
      result = tmp_result;
    } else if ( result.scoreAv() > tmp_result.scoreAv() ) {
      result = tmp_result;
    }
    delete evaluation;
  }
  ostringstream stats;
  // multiplication by 100 in order to keep the average precision
  // in the TER calculation.
  stats << result.numEdits*100.0 << " " << result.averageWords*100.0 << " " << result.scoreAv()*100.0 << " " ;
  string stats_str = stats.str();
  entry.set ( stats_str );
}

float TerScorer::calculateScore(const vector<int>& comps) const
{
  float denom = 1.0 * comps[1];
  float num =  -1.0 * comps[0];
  if ( denom == 0 ) {
//         shouldn't happen!
    return 1.0;
  } else {
    return (1.0+(num / denom));
  }
}

}
