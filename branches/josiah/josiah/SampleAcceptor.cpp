#include "SampleAcceptor.h"
#include "TranslationDelta.h"
#include "StaticData.h"
#include "GibbsOperator.h"

using namespace std;

namespace Josiah {
  
long MHAcceptor::mhtotal = 0;  
long MHAcceptor::acceptanceCtr = 0;  
  
void SampleAcceptor::getScores(const vector<TranslationDelta*>& deltas, vector<double>& scores) {
  for (vector<TranslationDelta*>::const_iterator i = deltas.begin(); i != deltas.end(); ++i) {
    scores.push_back((**i).getScore());
  }
}

void SampleAcceptor::normalize(vector<double>& scores) {
  double sum = scores[0];
  for (size_t i = 1; i < scores.size(); ++i) {
    sum = log_sum(sum,scores[i]);
  }
  transform(scores.begin(),scores.end(),scores.begin(),bind2nd(minus<double>(),sum));
}

double SampleAcceptor::getRandom() {
  double random =  RandomNumberGenerator::instance().next();//(double)rand() / RAND_MAX;
  random = log(random);
  return random;  
}

size_t SampleAcceptor::getSample(const vector<double>& scores, double random) {
  size_t position = 1;
  double sum = scores[0];
  for (; position < scores.size() && sum < random; ++position) {
    sum = log_sum(sum,scores[position]);
  }
  
  size_t chosen =  position-1;
  VERBOSE(3,"The chosen sample is " << chosen << endl);
  return chosen;
}

void SampleAcceptor::getNormalisedScores(const std::vector<TranslationDelta*>& deltas, std::vector<double>& scores) {
    //get the scores
    getScores(deltas, scores);
    normalize(scores);
}

void FixedTempAcceptor::getNormalisedScores(const std::vector<TranslationDelta*>& deltas, std::vector<double>& scores) {
    getScores(deltas, scores);
    IFVERBOSE(2) {
        cerr << "Before annealing, scores are :";
        copy(scores.begin(),scores.end(),ostream_iterator<double>(cerr," "));
        cerr << endl;
    }
  //do annealling
    transform(scores.begin(),scores.end(),scores.begin(),bind2nd(multiplies<double>(), 1.0/m_temp));
    IFVERBOSE(2) {
        cerr << "After annealing, scores are :";
        copy(scores.begin(),scores.end(),ostream_iterator<double>(cerr," "));
        cerr << endl;
    }
  
    normalize(scores);
}
  
TranslationDelta* SampleAcceptor::choose(const vector<TranslationDelta*>& deltas) {
  //get the scores
  vector<double> scores;
  getNormalisedScores(deltas,scores);
  
  double random = getRandom();
  size_t chosen = getSample(scores, random);
  
  return deltas[chosen];
}



TranslationDelta* GreedyAcceptor::choose(const vector<TranslationDelta*>& deltas) {
  size_t chosen = maxScore(deltas);
  return deltas[chosen];
}


size_t GreedyAcceptor::maxScore(const vector<TranslationDelta*>& deltas) {
  size_t best(0);
  float bestScore = -1e10;
  float score;
  
  for (vector<TranslationDelta*>::const_iterator i = deltas.begin(); i != deltas.end(); ++i) {
    score = (**i).getScore();
    if (score > bestScore) {
      bestScore = score;
      best =  i - deltas.begin();
    }
  }
  return best;
}
  
TranslationDelta* MHAcceptor::choose(TranslationDelta* currSample, TranslationDelta* nextSample) {
  //First, get scores using proposal distribution 
  float nextSampleProposalScore =  nextSample->getScore();
  float currSampleProposalScore =  currSample->getScore();
  
  VERBOSE(2, "Curr Sample Prop Score " << currSample->getScores() << " : " << currSample->getScore() << endl)
  VERBOSE(2, "Next Sample Prop Score " << nextSample->getScores() << " : " << nextSample->getScore() << endl)
  //Update the score producer
  currSample->getOperator()->setGibbsLMInfo(m_targetLMInfo);
  
  
  //Now calculate scores using target distribution
  TranslationDelta* nextTargetSample = nextSample->Create();
  TranslationDelta* currTargetSample = currSample->Create();
  
  float nextSampleTargetScore = nextTargetSample->getScore();
  float currSampleTargetScore = currTargetSample->getScore();

  VERBOSE(2, "Curr Sample Target Score " << currTargetSample->getScores() << " : " << currTargetSample->getScore() << endl)
  VERBOSE(2, "Next Sample Target Score " << nextTargetSample->getScores() << " : " << nextTargetSample->getScore() << endl)

  //Restore the score producer
  currSample->getOperator()->setGibbsLMInfo(m_proposalLMInfo);
  
  //Calculate a
  float a = (nextSampleProposalScore + nextSampleTargetScore) - (currSampleProposalScore + currSampleTargetScore);
  
  //Copy modified fvs back
  currSample->setScores(currTargetSample->getScores());
  nextSample->setScores(nextTargetSample->getScores());
  currSample->updateWeightedScore();
  nextSample->updateWeightedScore();
  
  //Delete samples
  delete nextTargetSample;
  delete currTargetSample;
  
  
  VERBOSE (2, " A : " << a << endl)
  //Accept/reject
  mhtotal++;
  if (a >= 0) { //accept
    acceptanceCtr++;
    return nextSample; 
  }
  else {
    double random =  RandomNumberGenerator::instance().next();
    random = log(random);
    VERBOSE (2, " Random : " << random << endl)  
    if (a >= random) //accept with prob random
      return nextSample;
    else
      return currSample; //reject 
  }
}  
  

size_t BestNeighbourTgtAssigner::getTarget(const vector<TranslationDelta*>& deltas, const TranslationDelta* noChangeDelta) {
  //Only do best neighbour for the moment
  float bestGain = -1;
  int bestGainIndex = -1;
  for (vector<TranslationDelta*>::const_iterator i = deltas.begin(); i != deltas.end(); ++i) {
    if ((*i)->getGain() > bestGain) {
      bestGain = (*i)->getGain();
      bestGainIndex = i - deltas.begin();
    }
  }
  IFVERBOSE(1) {
    if (bestGainIndex > -1) {
      cerr << "best nbr has score " << deltas[bestGainIndex]->getScore() << " and gain " << deltas[bestGainIndex]->getGain() << endl;
      //cerr << "No change has score " << noChangeDelta->getScore() << " and gain " << noChangeDelta->getGain() << endl;    
    }
  }
  return bestGainIndex;
}

size_t ClosestBestNeighbourTgtAssigner::getTarget(const vector<TranslationDelta*>& deltas, const TranslationDelta* chosenDelta) {
  //Only do best neighbour for the moment
  float minScoreDiff = 10e10;
  int closestBestNbr = -1;
  float chosenGain = chosenDelta->getGain();
  float chosenScore = chosenDelta->getScore(); 
  for (vector<TranslationDelta*>::const_iterator i = deltas.begin(); i != deltas.end(); ++i) {
    if ((*i)->getGain() > chosenGain ) {
      float scoreDiff = chosenScore -  (*i)->getScore();
      if (scoreDiff < minScoreDiff) {
        minScoreDiff = scoreDiff;
        closestBestNbr = i - deltas.begin();
      }
    }
  }
  
  return closestBestNbr;
}
}

