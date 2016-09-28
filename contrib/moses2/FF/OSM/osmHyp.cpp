#include "osmHyp.h"
#include <sstream>

using namespace std;
using namespace lm::ngram;

namespace Moses2
{
void osmState::setState(const lm::ngram::State & val)
{
  j = 0;
  E = 0;
  lmState = val;
}

void osmState::saveState(int jVal, int eVal, map <int , string> & gapVal)
{
  gap.clear();
  gap = gapVal;
  j = jVal;
  E = eVal;
}

size_t osmState::hash() const
{
  size_t ret = j;

  boost::hash_combine(ret, E);
  boost::hash_combine(ret, gap);
  boost::hash_combine(ret, lmState.length);

  return ret;
}

bool osmState::operator==(const FFState& otherBase) const
{
  const osmState &other = static_cast<const osmState&>(otherBase);
  if (j != other.j)
    return false;
  if (E != other.E)
    return false;
  if (gap != other.gap)
    return false;
  if (lmState.length != other.lmState.length)
    return false;

  return true;
}

std::string osmState :: getName() const
{

  return "done";
}

//////////////////////////////////////////////////

osmHypothesis :: osmHypothesis()
{
  opProb = 0;
  gapWidth = 0;
  gapCount = 0;
  openGapCount = 0;
  deletionCount = 0;
  gapCount = 0;
  j = 0;
  E = 0;
  gap.clear();
}

void osmHypothesis :: setState(const FFState* prev_state)
{

  if(prev_state != NULL) {

    j = static_cast <const osmState *> (prev_state)->getJ();
    E =  static_cast <const osmState *> (prev_state)->getE();
    gap = static_cast <const osmState *> (prev_state)->getGap();
    lmState = static_cast <const osmState *> (prev_state)->getLMState();
  }
}

void osmHypothesis :: saveState(osmState &state)
{
  state.setState(lmState);
  state.saveState(j,E,gap);
}

int osmHypothesis :: isTranslationOperation(int x)
{
  if (operations[x].find("_JMP_BCK_") != -1)
    return 0;

  if (operations[x].find("_JMP_FWD_") != -1)
    return 0;

  if (operations[x].find("_CONT_CEPT_") != -1)
    return 0;

  if (operations[x].find("_INS_GAP_") != -1)
    return 0;

  return 1;

}

void osmHypothesis :: removeReorderingOperations()
{
  gapCount = 0;
  deletionCount = 0;
  openGapCount = 0;
  gapWidth = 0;

  std::vector <std::string> tupleSequence;

  for (int x = 0; x < operations.size(); x++) {
    // cout<<operations[x]<<endl;

    if(isTranslationOperation(x) == 1) {
      tupleSequence.push_back(operations[x]);
    }

  }

  operations.clear();
  operations = tupleSequence;
}

void osmHypothesis :: calculateOSMProb(OSMLM& ptrOp)
{

  opProb = 0;
  State currState = lmState;
  State temp;

  for (size_t i = 0; i<operations.size(); i++) {
    temp = currState;
    opProb += ptrOp.Score(temp,operations[i],currState);
  }

  lmState = currState;

  //print();
}


int osmHypothesis :: firstOpenGap(vector <int> & coverageVector)
{

  int firstOG =-1;

  for(int nd = 0; nd < coverageVector.size(); nd++) {
    if(coverageVector[nd]==0) {
      firstOG = nd;
      return firstOG;
    }
  }

  return firstOG;

}

string osmHypothesis :: intToString(int num)
{
  return SPrint(num);

}

void osmHypothesis :: generateOperations(int & startIndex , int j1 , int contFlag , Bitmap & coverageVector , string english , string german , set <int> & targetNullWords , vector <string> & currF)
{

  int gFlag = 0;
  int gp = 0;
  int ans;


  if ( j < j1) { // j1 is the index of the source word we are about to generate ...
    //if(coverageVector[j]==0) // if source word at j is not generated yet ...
    if(coverageVector.GetValue(j)==0) { // if source word at j is not generated yet ...
      operations.push_back("_INS_GAP_");
      gFlag++;
      gap[j]="Unfilled";
    }
    if (j == E) {
      j = j1;
    } else {
      operations.push_back("_JMP_FWD_");
      j=E;
    }
  }

  if (j1 < j) {
    // if(j < E && coverageVector[j]==0)
    if(j < E && coverageVector.GetValue(j)==0) {
      operations.push_back("_INS_GAP_");
      gFlag++;
      gap[j]="Unfilled";
    }

    j=closestGap(gap,j1,gp);
    operations.push_back("_JMP_BCK_"+ intToString(gp));

    //cout<<"I am j "<<j<<endl;
    //cout<<"I am j1 "<<j1<<endl;

    if(j==j1)
      gap[j]="Filled";
  }

  if (j < j1) {
    operations.push_back("_INS_GAP_");
    gap[j] = "Unfilled";
    gFlag++;
    j=j1;
  }

  if(contFlag == 0) { // First words of the multi-word cept ...

    if(english == "_TRANS_SLF_") { // Unknown word ...
      operations.push_back("_TRANS_SLF_");
    } else {
      operations.push_back("_TRANS_" + english + "_TO_" + german);
    }

    //ans = firstOpenGap(coverageVector);
    ans = coverageVector.GetFirstGapPos();

    if (ans != -1)
      gapWidth += j - ans;

  } else if (contFlag == 2) {

    operations.push_back("_INS_" + german);
    ans = coverageVector.GetFirstGapPos();

    if (ans != -1)
      gapWidth += j - ans;
    deletionCount++;
  } else {
    operations.push_back("_CONT_CEPT_");
  }

  //coverageVector[j]=1;
  coverageVector.SetValue(j,1);
  j+=1;

  if(E<j)
    E=j;

  if (gFlag > 0)
    gapCount++;

  openGapCount += getOpenGaps();

  //if (coverageVector[j] == 0 && targetNullWords.find(j) != targetNullWords.end())
  if (j < coverageVector.GetSize()) {
    if (coverageVector.GetValue(j) == 0 && targetNullWords.find(j) != targetNullWords.end()) {
      j1 = j;
      german = currF[j1-startIndex];
      english = "_INS_";
      generateOperations(startIndex, j1, 2 , coverageVector , english , german , targetNullWords , currF);
    }
  }

}

void osmHypothesis :: print()
{
  for (int i = 0; i< operations.size(); i++) {
    cerr<<operations[i]<<" ";

  }

  cerr<<endl<<endl;

  cerr<<"Operation Probability "<<opProb<<endl;
  cerr<<"Gap Count "<<gapCount<<endl;
  cerr<<"Open Gap Count "<<openGapCount<<endl;
  cerr<<"Gap Width "<<gapWidth<<endl;
  cerr<<"Deletion Count "<<deletionCount<<endl;

  cerr<<"_______________"<<endl;
}

int osmHypothesis :: closestGap(map <int,string> gap, int j1, int & gp)
{

  int dist=1172;
  int value=-1;
  int temp=0;
  gp=0;
  int opGap=0;

  map <int,string> :: iterator iter;

  iter=gap.end();

  do {
    iter--;
    //cout<<"Trapped "<<iter->first<<endl;

    if(iter->first==j1 && iter->second== "Unfilled") {
      opGap++;
      gp = opGap;
      return j1;

    }

    if(iter->second =="Unfilled") {
      opGap++;
      temp = iter->first - j1;

      if(temp<0)
        temp=temp * -1;

      if(dist>temp && iter->first < j1) {
        dist=temp;
        value=iter->first;
        gp=opGap;
      }
    }


  } while(iter!=gap.begin());

  return value;
}



int osmHypothesis :: getOpenGaps()
{
  map <int,string> :: iterator iter;

  int nd = 0;
  for (iter = gap.begin(); iter!=gap.end(); iter++) {
    if(iter->second == "Unfilled")
      nd++;
  }

  return nd;

}

void osmHypothesis :: generateDeleteOperations(std::string english, int currTargetIndex, std::set <int> doneTargetIndexes)
{

  operations.push_back("_DEL_" + english);
  currTargetIndex++;

  while(doneTargetIndexes.find(currTargetIndex) != doneTargetIndexes.end()) {
    currTargetIndex++;
  }

  if (sourceNullWords.find(currTargetIndex) != sourceNullWords.end()) {
    english = currE[currTargetIndex];
    generateDeleteOperations(english,currTargetIndex,doneTargetIndexes);
  }

}

void osmHypothesis :: computeOSMFeature(int startIndex , Bitmap & coverageVector)
{

  set <int> doneTargetIndexes;
  set <int> eSide;
  set <int> fSide;
  set <int> :: iterator iter;
  string english;
  string source;
  int j1;
  int targetIndex = 0;
  doneTargetIndexes.clear();


  if (targetNullWords.size() != 0) { // Source words to be deleted in the start of this phrase ...
    iter = targetNullWords.begin();

    if (*iter == startIndex) {

      j1 = startIndex;
      source = currF[j1-startIndex];
      english = "_INS_";
      generateOperations(startIndex, j1, 2 , coverageVector , english , source , targetNullWords , currF);
    }
  }

  if (sourceNullWords.find(targetIndex) != sourceNullWords.end()) { // first word has to be deleted ...
    english = currE[targetIndex];
    generateDeleteOperations(english,targetIndex, doneTargetIndexes);
  }


  for (size_t i = 0; i < ceptsInPhrase.size(); i++) {
    source = "";
    english = "";

    fSide = ceptsInPhrase[i].first;
    eSide = ceptsInPhrase[i].second;

    iter = eSide.begin();
    targetIndex = *iter;
    english += currE[*iter];
    iter++;

    for (; iter != eSide.end(); iter++) {
      if(*iter == targetIndex+1)
        targetIndex++;
      else
        doneTargetIndexes.insert(*iter);

      english += "^_^";
      english += currE[*iter];
    }

    iter = fSide.begin();
    source += currF[*iter];
    iter++;

    for (; iter != fSide.end(); iter++) {
      source += "^_^";
      source += currF[*iter];
    }

    iter = fSide.begin();
    j1 = *iter + startIndex;
    iter++;

    generateOperations(startIndex, j1, 0 , coverageVector , english , source , targetNullWords , currF);


    for (; iter != fSide.end(); iter++) {
      j1 = *iter + startIndex;
      generateOperations(startIndex, j1, 1 , coverageVector , english , source , targetNullWords , currF);
    }

    targetIndex++; // Check whether the next target word is unaligned ...

    while(doneTargetIndexes.find(targetIndex) != doneTargetIndexes.end()) {
      targetIndex++;
    }

    if(sourceNullWords.find(targetIndex) != sourceNullWords.end()) {
      english = currE[targetIndex];
      generateDeleteOperations(english,targetIndex, doneTargetIndexes);
    }
  }

  //removeReorderingOperations();

  //print();

}

void osmHypothesis :: getMeCepts ( set <int> & eSide , set <int> & fSide , map <int , vector <int> > & tS , map <int , vector <int> > & sT)
{
  set <int> :: iterator iter;

  int sz = eSide.size();
  vector <int> t;

  for (iter = eSide.begin(); iter != eSide.end(); iter++) {
    t = tS[*iter];

    for (size_t i = 0; i < t.size(); i++) {
      fSide.insert(t[i]);
    }

  }

  for (iter = fSide.begin(); iter != fSide.end(); iter++) {

    t = sT[*iter];

    for (size_t i = 0 ; i<t.size(); i++) {
      eSide.insert(t[i]);
    }

  }

  if (eSide.size () > sz) {
    getMeCepts(eSide,fSide,tS,sT);
  }

}

void osmHypothesis :: constructCepts(vector <int> & align , int startIndex , int endIndex, int targetPhraseLength)
{

  std::map <int , vector <int> > sT;
  std::map <int , vector <int> > tS;
  std::set <int> eSide;
  std::set <int> fSide;
  std::set <int> :: iterator iter;
  std :: map <int , vector <int> > :: iterator iter2;
  std :: pair < set <int> , set <int> > cept;
  int src;
  int tgt;


  for (size_t i = 0;  i < align.size(); i+=2) {
    src = align[i];
    tgt = align[i+1];
    tS[tgt].push_back(src);
    sT[src].push_back(tgt);
  }

  for (int i = startIndex; i<= endIndex; i++) { // What are unaligned source words in this phrase ...
    if (sT.find(i-startIndex) == sT.end()) {
      targetNullWords.insert(i);
    }
  }

  for (int i = 0; i < targetPhraseLength; i++) { // What are unaligned target words in this phrase ...
    if (tS.find(i) == tS.end()) {
      sourceNullWords.insert(i);
    }
  }


  while (tS.size() != 0 && sT.size() != 0) {

    iter2 = tS.begin();

    eSide.clear();
    fSide.clear();
    eSide.insert (iter2->first);

    getMeCepts(eSide, fSide, tS , sT);

    for (iter = eSide.begin(); iter != eSide.end(); iter++) {
      iter2 = tS.find(*iter);
      tS.erase(iter2);
    }

    for (iter = fSide.begin(); iter != fSide.end(); iter++) {
      iter2 = sT.find(*iter);
      sT.erase(iter2);
    }

    cept = make_pair (fSide , eSide);
    ceptsInPhrase.push_back(cept);
  }



  /*

      cerr<<"Extracted Cepts "<<endl;
      for (int i = 0; i < ceptsInPhrase.size(); i++)
        {

          fSide = ceptsInPhrase[i].first;
          eSide = ceptsInPhrase[i].second;

          for (iter = eSide.begin(); iter != eSide.end(); iter++)
          {
              cerr<<*iter<<" ";
          }
              cerr<<"<---> ";

          for (iter = fSide.begin(); iter != fSide.end(); iter++)
          {
            cerr<<*iter<<" ";
          }

          cerr<<endl;
        }
        cerr<<endl;

      cerr<<"Unaligned Target Words"<<endl;

      for (iter = sourceNullWords.begin(); iter != sourceNullWords.end(); iter++)
        cerr<<*iter<<"<--->"<<endl;

      cerr<<"Unaligned Source Words"<<endl;

      for (iter = targetNullWords.begin(); iter != targetNullWords.end(); iter++)
        cerr<<*iter<<"<--->"<<endl;

  */

}

void osmHypothesis :: populateScores(vector <float> & scores , const int numFeatures)
{
  scores.clear();
  scores.push_back(opProb);

  if (numFeatures == 1)
    return;

  scores.push_back(gapWidth);
  scores.push_back(gapCount);
  scores.push_back(openGapCount);
  scores.push_back(deletionCount);
}


} // namespace

