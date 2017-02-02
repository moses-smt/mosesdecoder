#include "InterpolatedScorer.h"
#include "ScorerFactory.h"
#include "Util.h"

using namespace std;

namespace MosesTuning
{


// TODO: This is too long. Consider creating a function for
// initialization such as Init().
InterpolatedScorer::InterpolatedScorer(const string& name, const string& config)
  : Scorer(name,config)
{
  // name would be: HAMMING,BLEU or similar
  string scorers = name;
  while (scorers.length() > 0) {
    string scorertype = "";
    getNextPound(scorers, scorertype,",");
    Scorer *scorer = ScorerFactory::getScorer(scorertype,config);
    m_scorers.push_back(scorer);
  }
  if (m_scorers.size() == 0) {
    throw runtime_error("There are no scorers");
  }
  cerr << "Number of scorers: " << m_scorers.size() << endl;

  //TODO debug this
  string wtype = getConfig("weights","");
  //Default weights set to uniform ie. if two weights 0.5 each
  //weights should add to 1
  if (wtype.length() == 0) {
    float weight = 1.0 / m_scorers.size() ;
    //cout << " Default weights:" << weight << endl;
    for (size_t i = 0; i < m_scorers.size(); i ++) {
      m_scorer_weights.push_back(weight);
    }
  } else {
    float tot=0;
    //cout << "Defined weights:"  << endl;
    while (wtype.length() > 0) {
      string scoreweight = "";
      getNextPound(wtype,scoreweight,"+");
      float weight = atof(scoreweight.c_str());
      m_scorer_weights.push_back(weight);
      tot += weight;
      //cout << " :" << weight ;
    }
    //cout << endl;
    if (tot != float(1)) { // TODO: fix this checking in terms of readability.
      for (vector<float>::iterator it = m_scorer_weights.begin();
           it != m_scorer_weights.end(); ++it) {
        *it /= tot;
      }
    }

    if (m_scorers.size() != m_scorer_weights.size()) {
      throw runtime_error("The number of weights does not equal the number of scorers!");
    }
  }
  cerr << "The weights for the interpolated scorers are: " << endl;
  for (vector<float>::iterator it = m_scorer_weights.begin(); it < m_scorer_weights.end(); it++) {
    cerr << *it << " " ;
  }
  cerr <<endl;
}

bool InterpolatedScorer::useAlignment() const
{
  //cout << "InterpolatedScorer::useAlignment" << endl;
  for (vector<Scorer*>::const_iterator itsc =  m_scorers.begin(); itsc < m_scorers.end(); itsc++) {
    if ((*itsc)->useAlignment()) {
      //cout <<"InterpolatedScorer::useAlignment Returning true"<<endl;
      return true;
    }
  }
  return false;
};

void InterpolatedScorer::setScoreData(ScoreData* data)
{
  size_t last = 0;
  m_score_data = data;
  for (ScopedVector<Scorer>::iterator itsc = m_scorers.begin();
       itsc != m_scorers.end(); ++itsc) {
    int numScoresScorer = (*itsc)->NumberOfScores();
    ScoreData* newData =new ScoreData(*itsc);
    for (size_t i = 0; i < data->size(); i++) {
      ScoreArray scoreArray = data->get(i);
      ScoreArray newScoreArray;
      size_t numNBest = scoreArray.size();
      //cout << " Datasize " << data->size() <<  " NumNBest " << numNBest << endl ;
      for (size_t j = 0; j < numNBest ; j++) {
        ScoreStats scoreStats = data->get(i, j);
        //cout << "Scorestats " << scoreStats << " i " << i << " j " << j << endl;
        ScoreStats newScoreStats;
        for (size_t k = last; k < size_t(numScoresScorer + last); k++) {
          ScoreStatsType score = scoreStats.get(k);
          newScoreStats.add(score);
        }
        //cout << " last " << last << " NumScores " << numScoresScorer << "newScorestats " << newScoreStats << endl;
        newScoreArray.add(newScoreStats);
      }
      newScoreArray.setIndex(i);
      newData->add(newScoreArray);
    }
    //newData->dump();

    // NOTE: This class takes the ownership of the heap allocated
    // ScoreData objects to avoid the memory leak issues.
    m_scorers_score_data.push_back(newData);

    (*itsc)->setScoreData(newData);
    last += numScoresScorer;
  }
}


/** The interpolated scorer calls a vector of scorers and combines them with
    weights **/
void InterpolatedScorer::score(const candidates_t& candidates, const diffs_t& diffs,
                               statscores_t& scores) const
{
  //cout << "*******InterpolatedScorer::score" << endl;
  size_t scorerNum = 0;
  for (ScopedVector<Scorer>::const_iterator itsc = m_scorers.begin();
       itsc != m_scorers.end(); ++itsc) {
    //int numScores = (*itsc)->NumberOfScores();
    statscores_t tscores;
    (*itsc)->score(candidates,diffs,tscores);
    size_t inc = 0;
    for (statscores_t::iterator itstatsc = tscores.begin();
         itstatsc != tscores.end(); ++itstatsc) {
      //cout << "Scores " << (*itstatsc) << endl;
      float weight = m_scorer_weights[scorerNum];
      if (weight == 0) {
        stringstream msg;
        msg << "No weights for scorer" << scorerNum ;
        throw runtime_error(msg.str());
      }
      if (scorerNum == 0) {
        scores.push_back(weight * (*itstatsc));
      } else {
        scores[inc] +=  weight * (*itstatsc);
      }
      //cout << "Scorer:" << scorerNum <<  " scoreNum:" << inc << " score: " << (*itstatsc) << " weight:" << weight << endl;
      inc++;

    }
    scorerNum++;
  }

}

/** Interpolated scorer gets a vector of sufficient statistics, calls all scorers with corresponding statistics,
    and combines them with weights **/
float InterpolatedScorer::calculateScore(const std::vector<ScoreStatsType>& totals) const
{
  size_t scorerNum = 0;
  size_t last = 0;
  float score = 0;
  for (ScopedVector<Scorer>::const_iterator itsc = m_scorers.begin();
       itsc != m_scorers.end(); ++itsc) {
    int numScoresScorer = (*itsc)->NumberOfScores();
    std::vector<ScoreStatsType> totals_scorer(totals.begin()+last, totals.begin()+last+numScoresScorer);
    score += (*itsc)->calculateScore(totals_scorer) * m_scorer_weights[scorerNum];
    last += numScoresScorer;
    scorerNum++;
  }
  return score;
}


float InterpolatedScorer::getReferenceLength(const std::vector<ScoreStatsType>& totals) const
{
  size_t scorerNum = 0;
  size_t last = 0;
  float refLen = 0;
  for (ScopedVector<Scorer>::const_iterator itsc = m_scorers.begin();
       itsc != m_scorers.end(); ++itsc) {
    int numScoresScorer = (*itsc)->NumberOfScores();
    std::vector<ScoreStatsType> totals_scorer(totals.begin()+last, totals.begin()+last+numScoresScorer);
    refLen += (*itsc)->getReferenceLength(totals_scorer) * m_scorer_weights[scorerNum];
    last += numScoresScorer;
    scorerNum++;
  }
  return refLen;
}

void InterpolatedScorer::setReferenceFiles(const vector<string>& referenceFiles)
{
  for (ScopedVector<Scorer>::iterator itsc = m_scorers.begin();
       itsc != m_scorers.end(); ++itsc) {
    (*itsc)->setReferenceFiles(referenceFiles);
  }
}

void InterpolatedScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry)
{
  stringstream buff;
  string align = text;
  string sentence = text;
  size_t alignmentData = text.find("|||");
  //Get sentence and alignment parts
  if(alignmentData != string::npos) {
    getNextPound(align,sentence, "|||");
  }

  int i = 0;
  for (ScopedVector<Scorer>::iterator itsc = m_scorers.begin(); itsc != m_scorers.end(); ++itsc) {
    ScoreStats tempEntry;
    if ((*itsc)->useAlignment()) {
      (*itsc)->prepareStats(sid, text, tempEntry);
    } else {
      (*itsc)->prepareStats(sid, sentence, tempEntry);
    }
    if (i > 0) buff <<  " ";
    buff << tempEntry;
    i++;
  }
  //cout << " Scores for interpolated: " << buff << endl;
  string str = buff.str();
  entry.set(str);
}

void InterpolatedScorer::setFactors(const string& factors)
{
  if (factors.empty()) return;

  vector<string> fsplit;
  split(factors, ',', fsplit);

  if (fsplit.size() != m_scorers.size())
    throw runtime_error("Number of factor specifications does not equal number of interpolated scorers.");

  for (size_t i = 0; i < m_scorers.size(); ++i) {
    m_scorers[i]->setFactors(fsplit[i]);
  }
}

void InterpolatedScorer::setFilter(const string& filterCommand)
{
  if (filterCommand.empty()) return;

  vector<string> csplit;
  split(filterCommand, ',', csplit);

  if (csplit.size() != m_scorers.size())
    throw runtime_error("Number of command specifications does not equal number of interpolated scorers.");

  for (size_t i = 0; i < m_scorers.size(); ++i) {
    m_scorers[i]->setFilter(csplit[i]);
  }
}

}
