/*
########################################################################################

Transliteration Mining - A Program to Extract Transliteration Pairs from
a bilingual word list
Source Contributor: Nadir Durrani

########################################################################################

*/

#include <cstdlib>
#include <map>
#include <set>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>


using namespace std;


double initTransitionProb;
double LAMBDA;

double addLogProbs(double A , double B)   // this function adds probabilities ...
{

  if (A == B)
    return (A + log10(2.0));

  if (A > B) {
    if (A - B > 6)	// A is a lot bigger ...
      return A;
    else
      return (A + log10(1+pow(10,(B-A))));
  }

  else {	// B > A
    if (B - A > 6)
      return B;
    else
      return (B + log10(1+pow(10,(A-B))));
  }

}


class NodeStructure
{

public:

  NodeStructure() {};
  NodeStructure(vector <string> & s , vector <string> & t);
  double getPosterior() {
    return PPR;
  }
  void computeFwdBckProbs(map <string , double> & gammas, map <string, double> & alignmentCounts);
  void computeNonTransliterationProb (map <string , double> & sourceUnigrams , map <string , double> & targetUnigrams);
  void print();

  vector <string> source;
  vector <string> target;
  ~NodeStructure() {};

private:

  double NTR;			// Non-transliteration probability of a sentence pair ...
  double PPR;			// Posterior Probability ...
  double ALPHA;
  double BETA;

  void computeGammaForEdges(map < pair <int , int> , double > & parents, map < pair <int , int> , double > & children , map <string, double> & transitionProbs , map <string, double> & alignmentCounts);
  double computeFwdProbs(pair <int , int> & ST, map <string , double> & gammas, map < pair <int , int> , double > & parents);
  double FwdProb (pair <int , int> & TS, map <string , double> & gammas, map < pair <int , int> , double > & parents);
  double BckProb (pair <int , int> & TS, map <string , double> & gammas, map < pair <int , int> , double > & chidren);
  double computeBckProbs(pair <int , int> & ST, map <string , double> & gammas, map < pair <int , int> , double > & children);
  void getIncomingEdges (pair <int , int> & ST , vector < pair < int , int> > & incomingEdges);
  void getOutgoingEdges (pair <int , int> & ST , vector < pair < int , int> > & outgoingEdges);
  double  getTransitionProb(map <string, double> & transitionProbs , pair <int,int> & edge);
  void  updateAlignmentCount(map <string, double> & transitionProbs, map <string, double> & alignmentCounts , pair <int,int> & edge , double alpha , double beta);
  void computePosteriorProb();
  double scaleGamma(double g);
  void getEdge (pair <int , int> & v1 , pair <int , int> & v2 , pair <int , int> & v3);

};

void NodeStructure :: print()
{

  for (int i = 0; i < source.size(); i++)
    cout<<source[i];

  cout<<"\t";

  for (int i = 0; i < target.size(); i++)
    cout<<target[i];

  cout<<"\t"<<pow(10,PPR)<<endl;

}

NodeStructure :: NodeStructure(vector <string> & s , vector <string> & t)
{
  source = s;
  target = t;
}


void NodeStructure :: getEdge (pair <int , int> & v1 , pair <int , int> & v2 , pair <int , int> & v3)
{
  if (v2.first - v1.first == 0)
    v3.first = -1;
  else
    v3.first = v2.first;

  if (v2.second - v1.second == 0)
    v3.second = -1;
  else
    v3.second = v2.second;
}

void NodeStructure :: computeGammaForEdges(map < pair <int , int> , double > & parents, map < pair <int , int> , double > & children , map <string, double> & transitionProbs , map <string, double> & alignmentCounts)
{

  vector < pair < int , int> > incomingEdges;
  map < pair <int , int> , double > :: iterator cIter;
  map < pair <int , int> , double > :: iterator pIter;
  pair <int , int> ST = make_pair (-1,-1);
  pair <int , int> edge;

  children.erase(ST);
  double tProb;
  double alpha;
  double beta;

  for (cIter = children.begin(); cIter != children.end(); cIter++) {
    ST = cIter->first;

    getIncomingEdges (ST , incomingEdges);
    beta = cIter->second;

    for (int i = 0; i< incomingEdges.size(); i++) {
      pIter = parents.find(incomingEdges[i]);

      alpha = pIter->second;
      getEdge (incomingEdges[i] , ST , edge);

      updateAlignmentCount(transitionProbs, alignmentCounts , edge , alpha , beta);
    }
  }

}

void NodeStructure :: computeNonTransliterationProb (map <string , double> & sourceUnigrams , map <string , double> & targetUnigrams)
{

  NTR = 0.0;

  for (int i = 0; i < source.size(); i++) {
    NTR +=  sourceUnigrams[source[i]];
  }

  for (int i = 0; i < target.size(); i++) {

    NTR +=  targetUnigrams[target[i]];
  }

}

double NodeStructure :: scaleGamma(double g)
{
  double translit = log10 (1 - pow (10, PPR));
  return g + translit;
}
void NodeStructure :: computePosteriorProb()
{
  double LAMBDA2 = log10(1 - pow(10, LAMBDA));
  double transliterate = LAMBDA2 + ALPHA;	// Transliteration Prob ...
  double translate = LAMBDA + NTR;						// Translation Prob ...
  double trans = transliterate - translate;
  //cout<<LAMBDA<<" "<<LAMBDA2<<endl;
  //cout<<transliterate<<" "<<translate<<" "<<trans<<endl;
  //cout<<pow(10 , trans)<<endl;
  double prob = 1/(1+ pow(10 , trans));
  PPR = log10(prob);

  //cout<<"Posterior Prob "<<PPR<<endl;
}

void NodeStructure :: computeFwdBckProbs(map <string , double> & gammas , map <string, double> & alignmentCounts)
{
  pair <int , int> START = make_pair (source.size()-1 , target.size()-1);
  pair <int , int> END = make_pair (-1 , -1);

  map < pair <int , int> , double > parents;
  parents[make_pair(-1,-1)] = 0.0;
  map < pair <int , int> , double > children;
  children[make_pair(source.size()-1,target.size()-1)] = 0.0;

  ALPHA = computeFwdProbs(START , gammas, parents);
  BETA = computeBckProbs(END , gammas, children);

  computePosteriorProb();
  //cout<<"Alpha "<<ALPHA<<" Beta "<<BETA<<endl;
  computeGammaForEdges(parents , children , gammas , alignmentCounts);

}

void NodeStructure :: getIncomingEdges (pair <int , int> & ST , vector < pair < int , int> > & incomingEdges)
{
  incomingEdges.clear();

  if (ST.first == -1) {	// Source is NULL ..
    incomingEdges.push_back(make_pair(ST.first , ST.second-1));
  } else if (ST.second == -1) {	// Target is NULL ...
    incomingEdges.push_back(make_pair(ST.first-1 , ST.second));
  } else {
    incomingEdges.push_back(make_pair(ST.first , ST.second-1));
    incomingEdges.push_back(make_pair(ST.first-1 , ST.second));
    incomingEdges.push_back(make_pair(ST.first-1 , ST.second-1));
  }

}

void NodeStructure :: getOutgoingEdges (pair <int , int> & ST , vector < pair < int , int> > & outgoingEdges)
{

  if (ST.first == source.size()-1) {	// Source is END ..
    outgoingEdges.push_back(make_pair(ST.first , ST.second+1));
  } else if (ST.second == target.size()-1) {	// Target is END ...
    outgoingEdges.push_back(make_pair(ST.first+1 , ST.second));
  } else {
    outgoingEdges.push_back(make_pair(ST.first , ST.second+1));
    outgoingEdges.push_back(make_pair(ST.first+1 , ST.second));
    outgoingEdges.push_back(make_pair(ST.first+1 , ST.second+1));
  }

}

void NodeStructure :: updateAlignmentCount(map <string, double> & transitionProbs, map <string, double> & alignmentCounts , pair <int,int> & edge , double alpha , double beta)
{

  double tProb;
  double tgamma;
  double gamma;
  map <string , double> :: iterator aCounts;
  string query;

  if (edge.first == -1)
    query = "NULL";
  else
    query = source[edge.first];

  query += "-";

  if (edge.second == -1)
    query += "NULL";
  else
    query += target[edge.second];

  //cout<<" Query "<<query<<endl;
  if (transitionProbs.size() == 0)
    tProb = initTransitionProb;
  else
    tProb = transitionProbs[query];


  tgamma = alpha + tProb + beta - ALPHA;
  gamma = scaleGamma(tgamma);
  //cout<<alpha<<" "<<beta<<" "<<gamma<<endl;
  //cout<<tProb<<" "<<ALPHA<<endl;

  aCounts = alignmentCounts.find(query);

  if (aCounts == alignmentCounts.end()) {
    alignmentCounts[query] = gamma;
  } else {
    double temp = aCounts->second;
    aCounts->second = addLogProbs(temp , gamma);
  }

}

double NodeStructure :: getTransitionProb(map <string, double> & transitionProbs , pair <int,int> & edge)
{

  if (transitionProbs.size() == 0)
    return initTransitionProb;

  string query;

  if (edge.first == -1)
    query = "NULL";
  else
    query = source[edge.first];

  query += "-";

  if (edge.second == -1)
    query += "NULL";
  else
    query += target[edge.second];

  //cout<<" Query "<<query<<endl;
  return transitionProbs[query];
}

double NodeStructure :: FwdProb (pair <int , int> & TS, map <string , double> & gammas, map < pair <int , int> , double > & parents)
{

  double thisAlpha;
  double alpha = -2000;
  vector < pair < int , int> > incomingEdges;
  pair <int , int> edge;


  getIncomingEdges (TS , incomingEdges);

  for (int k = 0; k < incomingEdges.size(); k++) {
    thisAlpha = parents[incomingEdges[k]];
    getEdge (incomingEdges[k], TS , edge);
    thisAlpha += getTransitionProb(gammas , edge);		// Get Transition Prob ...
    double temp = alpha;
    alpha = addLogProbs(temp , thisAlpha);			// Sum of all parents * transition prob ..
    // cout<<temp<<"+"<<thisAlpha<<"="<<alpha<<endl;
  }

  return alpha;
}

double NodeStructure :: computeFwdProbs(pair <int , int> & ST, map <string , double> & gammas, map < pair <int , int> , double > & parents)
{

  pair <int , int> TS;
  double alpha;

  for (int i = 0; i < source.size(); i++) {
    TS = make_pair (i , -1);
    alpha = FwdProb (TS, gammas, parents);
    parents[TS] = alpha;
  }

  for (int i = 0; i < target.size(); i++) {
    TS = make_pair (-1 , i);
    alpha = FwdProb (TS, gammas, parents);
    parents[TS] = alpha;
  }

  for (int i = 0; i < source.size(); i++) {
    for (int j = 0; j < target.size(); j++) {
      TS = make_pair (i , j);
      alpha = FwdProb (TS, gammas, parents);
      parents[TS] = alpha;
    }
  }

  return parents[ST];
}

double NodeStructure :: BckProb (pair <int , int> & TS, map <string , double> & gammas, map < pair <int , int> , double > & children)
{

  double thisBeta;
  double beta = -2000;
  vector < pair < int , int> > outgoingEdges;
  pair <int , int> edge;

  getOutgoingEdges (TS , outgoingEdges);

  for (int k = 0; k < outgoingEdges.size(); k++) {
    thisBeta = children[outgoingEdges[k]];
    getEdge (TS , outgoingEdges[k], edge);
    thisBeta += getTransitionProb(gammas , edge);		// Get Transition Prob ...
    double temp = beta;
    beta = addLogProbs(temp , thisBeta);			// Sum of all parents * transition prob ..
    // cout<<temp<<"+"<<thisAlpha<<"="<<alpha<<endl;
  }

  return beta;
}


double NodeStructure :: computeBckProbs(pair <int , int> & ST, map <string , double> & gammas, map < pair <int , int> , double > & children)
{

  pair <int , int> TS;
  double beta;

  for (int i = source.size()-2; i >= -1; i--) {
    TS = make_pair (i , target.size()-1);
    beta = BckProb (TS, gammas, children);
    children[TS] = beta;
  }

  for (int i = target.size()-2; i >=-1; i--) {
    TS = make_pair (source.size()-1 , i);
    beta = BckProb (TS, gammas, children);
    children[TS] = beta;
  }

  for (int i = source.size()-2 ; i >= -1 ; i--) {
    for (int j = target.size()-2 ; j >= -1; j--) {
      TS = make_pair (i , j);
      beta = BckProb (TS, gammas, children);
      children[TS] = beta;
    }
  }

  return children[ST];
}



void loadInput(const char * fileName, vector <string> & input)
{

  /* This function loads a file into a vector of strings */

  ifstream sr (fileName);
  string line;

  if(sr.is_open()) {
    while(getline(sr , line )) {
      input.push_back(line);
    }

    sr.close();
  } else {
    cout<<"Unable to read "<<fileName<<endl;
    exit(1);
  }

}

void printGammas(map <string, double> & alignmentCounts)
{
  map <string , double> :: iterator aCounts;

  for (aCounts = alignmentCounts.begin(); aCounts != alignmentCounts.end(); aCounts++) {
    cout<<aCounts->first<<" "<<aCounts->second<<endl;
  }
}


void getWords(string s, vector <string> & currInput)
{

  /* This function splits a string into vector of strings using space character as a delimiter */

  istringstream iss(s);
  currInput.clear();
  do {
    string sub;
    iss >> sub;
    currInput.push_back(sub);

  } while (iss);

  currInput.pop_back();
}

double getInitTransitionProb(int sourceToken, int targetToken)
{
  double prod = sourceToken * targetToken;
  return log10(1/prod);
}

void runIteration(map <int , NodeStructure> & graph , map <string , double> & gammas , int size)
{

  map <string, double> alignmentCounts;
  map <int , NodeStructure> :: iterator i;
  map <string , double> :: iterator aCounts;
  double sum = -2000.0;
  double tPPR = -2000.0;

  for (i = graph.begin(); i != graph.end(); i++) {

    i->second.computeFwdBckProbs(gammas , alignmentCounts);
    double temp = tPPR;

    tPPR = addLogProbs(graph[i->first].getPosterior() , temp);

  }

  for (aCounts = alignmentCounts.begin(); aCounts != alignmentCounts.end(); aCounts++) {
    double temp = sum;
    sum = addLogProbs(aCounts->second, temp);
  }


  for (aCounts = alignmentCounts.begin(); aCounts != alignmentCounts.end(); aCounts++) { // Normalizing ...
    aCounts->second = aCounts->second - sum;
  }

  gammas.clear();
  gammas = alignmentCounts;

  LAMBDA = tPPR - log10(size);
}


void setNTRProbabilities(map <int , NodeStructure> & graph , map <string , double> & sourceTypes , map <string , double > & targetTypes, double sourceTokens, double targetTokens)
{

  map <string , double> :: iterator i;
  map <int , NodeStructure> :: iterator j;


  for (i = sourceTypes.begin(); i!= sourceTypes.end(); i++) {
    i->second = log10(i->second/sourceTokens);
  }

  for (i = targetTypes.begin(); i!= targetTypes.end(); i++) {
    i->second = log10(i->second/targetTokens);
  }


  for (j = graph.begin(); j != graph.end(); j++) {
    j->second.computeNonTransliterationProb(sourceTypes , targetTypes);
  }

}

void printPosterior(map <int , NodeStructure> & graph)
{

  map <int , NodeStructure> :: iterator i;

  for (i = graph.begin(); i != graph.end(); i++)
    graph[i->first].print();
}


int main(int argc, char * argv[])
{

  vector <string> input;
  vector <string> source;
  vector <string> target;
  map <string , double> sourceTypes;
  map <string , double> targetTypes;
  set < vector <string> > tgt;
  set < vector <string> > src;
  double sourceTokens = 0;
  double targetTokens = 0;
  map <int , NodeStructure> graph;
  map <string , double> gammas;

  loadInput(argv[1],input);

  cerr<<"Constructing Graph "<<endl;

  for(int i=0; i<input.size(); i+=2) {

    //cerr<<input[i]<<endl;
    //cerr<<input[i+1]<<endl;


    getWords(input[i],source);
    getWords(input[i+1],target);

    if (src.find(source) == src.end()) {
      for (int j = 0; j< source.size(); j++)
        sourceTypes[source[j]]++;
      src.insert(source);
      sourceTokens += source.size();
    }

    if (tgt.find(target) == tgt.end()) {
      for (int j = 0; j< target.size(); j++)
        targetTypes[target[j]]++;

      tgt.insert(target);
      targetTokens += target.size();
    }

    NodeStructure obj (source,target);
    graph[i] = obj;

  }

  setNTRProbabilities(graph, sourceTypes, targetTypes, sourceTokens, targetTokens);
  initTransitionProb = getInitTransitionProb(sourceTypes.size()+1, targetTypes.size()+1);

  LAMBDA = log10(0.5);


  for (int i = 0; i< 10; i++) {

    cerr<<"Computing Probs : iteration "<<i+1<<endl;
    runIteration(graph ,  gammas , input.size()/2);

  }

  printPosterior(graph);
  cerr<<"Finished..."<<endl;

  return 0;
}

