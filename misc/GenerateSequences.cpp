#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <map>
#include <set>
#include <cstdlib>


using namespace std;


int stringToInteger(string s)
{

  istringstream buffer(s);
  int some_int;
  buffer >> some_int;
  return some_int;
}

void loadInput(const char * fileName, vector <string> & input)
{

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

void getWords(string s, vector <string> & currInput)
{
  istringstream iss(s);
  currInput.clear();
  do {
    string sub;
    iss >> sub;
    currInput.push_back(sub);

  } while (iss);

  currInput.pop_back();
}

string getTranslation(int index, vector < pair <string , vector <int> > > & gCepts , vector <string> & currF , map <string,int> & singletons)
{

  string translation = "";

  vector <int> fSide = gCepts[index].second;
  vector <int> :: iterator iter;

  for (iter = fSide.begin(); iter != fSide.end(); iter++) {
    if (iter != fSide.begin())
      translation += "^_^";

    translation+= currF[*iter];
  }

  if(singletons.find(translation)==singletons.end()) {
    return "_TRANS_" + gCepts[index].first + "_TO_" + translation + " ";
  }

  else {

    return "_TRANS_SLF_ ";
  }
}



int closestGap(map <int,string> gap,int j1, int & gp)
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

    if(iter->first==j1 and iter->second=="Unfilled") {
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

  //cout<<"Out"<<endl;
  return value;
}


void generateStory(vector <pair <string , vector <int> > > & gCepts, set <int> & targetNullWords, vector<string> & currF, map <string,int> & singletons)
{

  int fl = 0;
  int i = 0;   // Current English string position
  int j = 0; // Current French Position
  int N = gCepts.size(); // Total number of English words
  int k = 0; // Number of already generate French words
  int E = 0; // Position after most rightward French word generate so far
  int j1 = 0;  // Next french translation;
  int Li =0; // Links of word i
  int Lj=0; // Links of word j
  map <int,int > generated;
  map <int,string> gap;
  map <int,int> :: iterator  iter;
  int gp=0;
  //vector <string> iterator :: iterF;

  while (targetNullWords.find(j) != targetNullWords.end()) {
    cout<<"_INS_"<<currF[j]<<" ";
    generated[j]=-1;  // This word is generated -1 means unlinked ...
    j=j+1;
  }

  while (i < gCepts.size() && gCepts[i].second.size() == 0) {
    cout<<"_DEL_"<<gCepts[i].first<<" ";
    i=i+1;
  }

  E=j; // Update the position of most rightward French word

  while (i<N) {

    //cout<<"I am sending to the link "<<i<<" with 0 "<<endl;
    //j1 = getLink(i,0,Li,k);

    Li = gCepts[i].second.size();
    j1 = gCepts[i].second[k];

    //cout<<"i = "<<i<<" j1 = "<<j1<<"  j = "<<j<<" E = "<<E<<endl;

    if(j<j1) { // reordering needed ...
      iter = generated.find(j);
      if( iter == generated.end()) { // fj is not generated ...
        cout<<"_INS_GAP_ ";
        gap[j] = "Unfilled";
      }

      if (j==E) {
        j=j1;
      } else {
        cout<<"_JMP_FWD_ ";
        j=E;
      }

    }

    if(j1<j) {
      iter = generated.find(j);
      if(j<E && iter == generated.end()) { // fj is not generated ...

        cout<<"_INS_GAP_ ";
        gap[j]="Unfilled";
      }

      j=closestGap(gap,j1,gp);
      //cout<<j<<endl;
      cout<<"_JMP_BCK_"<<gp<<" ";

      if(j==j1)
        gap[j]="Filled";

    }

    if(j<j1) {
      cout<<"_INS_GAP_ ";
      gap[j] = "Unfilled";
      j=j1;
    }

    if(k==0) {
      cout<<getTranslation(i, gCepts,currF,singletons);
    } else {
      cout<<"_CONT_CEPT_ ";
    }
    generated[j]=i;
    j=j+1;
    k=k+1;

    while(targetNullWords.find(j) != targetNullWords.end()) { // fj is unlinked word ...
      //cout<<"Came here"<<j<<k<<endl;
      cout<<"_INS_"<<currF[j]<<" ";
      generated[j]=-1;  // This word is generated -1 means unlinked ...
      j=j+1;
    }

    if(E<j)
      E=j;
    //cout<<" Li "<<Li<<endl;
    if(k==Li) {
      i=i+1;
      k=0;

      while(i < gCepts.size() && gCepts[i].second.size() == 0) { // ei is unliked word ...
        cout<<"_DEL_"<<gCepts[i].first<<" ";
        i=i+1;

      }

    }

  }

  cout<<endl;
}



void ceptsInGenerativeStoryFormat(vector < pair < set <int> , set <int> > > & ceptsInPhrase , vector < pair < string , vector <int> > > & gCepts , set <int> & sourceNullWords, vector <string> & currE)
{

  gCepts.clear();
  set <int> eSide;
  set <int> fSide;
  std::set <int> :: iterator iter;
  string english;
  vector <int> germanIndex;
  int engIndex = 0;
  int prev=0;
  int curr;
  set <int> engDone;


  for (int i = 0; i< ceptsInPhrase.size(); i++) {
    english = "";
    germanIndex.clear();
    fSide = ceptsInPhrase[i].first;
    eSide = ceptsInPhrase[i].second;


    while(engIndex < *eSide.begin()) {
      // cout<<engIndex<<" "<<*eSide.begin()<<endl;

      while(engDone.find(engIndex) != engDone.end())
        engIndex++;

      while(sourceNullWords.find(engIndex) != sourceNullWords.end()) {
        english = currE[engIndex];
        engIndex++;
        gCepts.push_back(make_pair (english , germanIndex));
        english = "";
      }
    }

    for (iter = eSide.begin(); iter != eSide.end(); iter++) {
      curr = *iter;

      if(iter != eSide.begin()) {
        english += "^_^";

        if (prev == curr-1) {
          prev++;
          engIndex++;
        } else
          engDone.insert(curr);
      } else {
        prev = curr;
        //engIndex++;
        engIndex = prev+1;
      }
      english +=currE[curr];

    }

    for (iter = fSide.begin(); iter != fSide.end(); iter++) {
      germanIndex.push_back(*iter);
    }

    gCepts.push_back(make_pair (english , germanIndex));
    //	cout<<engIndex<<endl;

  }

  english = "";
  germanIndex.clear();

  //for (int i = 0; i< currE.size(); i++)
  // cout<<i<<" "<<currE[i]<<endl;

  while(engIndex < currE.size()) {
    // cout<<engIndex<<" "<<currE.size()-1<<endl;
    while(engDone.find(engIndex) != engDone.end())
      engIndex++;

    while(sourceNullWords.find(engIndex) != sourceNullWords.end()) {
      english = currE[engIndex];
      //cout<<"Here "<<engIndex<<english<<" "<<germanIndex.size()<<endl;
      engIndex++;
      gCepts.push_back(make_pair (english , germanIndex));
      english = "";
    }
  }

}

void printCepts(vector < pair < string , vector <int> > > & gCepts , vector <string> & currF)
{

  string eSide;
  vector <int> fSide;

  for (int i = 0; i < gCepts.size(); i++) {

    fSide = gCepts[i].second;
    eSide = gCepts[i].first;

    cout<<eSide;
    cout<<" <---> ";

    for (int j = 0; j < fSide.size(); j++) {
      cout<<currF[fSide[j]]<<" ";
    }

    cout<<endl;
  }

}

void getMeCepts ( set <int> & eSide , set <int> & fSide , map <int , vector <int> > & tS , map <int , vector <int> > & sT)
{
  set <int> :: iterator iter;

  int sz = eSide.size();
  vector <int> t;

  for (iter = eSide.begin(); iter != eSide.end(); iter++) {
    t = tS[*iter];

    for (int i = 0; i < t.size(); i++) {
      fSide.insert(t[i]);
    }

  }

  for (iter = fSide.begin(); iter != fSide.end(); iter++) {

    t = sT[*iter];

    for (int i = 0 ; i<t.size(); i++) {
      eSide.insert(t[i]);
    }

  }

  if (eSide.size () > sz) {
    getMeCepts(eSide,fSide,tS,sT);
  }

}

void constructCepts(vector < pair < set <int> , set <int> > > & ceptsInPhrase, set <int> & sourceNullWords, set <int> & targetNullWords, vector <string> & alignment, int eSize, int fSize)
{

  ceptsInPhrase.clear();
  sourceNullWords.clear();
  targetNullWords.clear();

  vector <int> align;

  std::map <int , vector <int> > sT;
  std::map <int , vector <int> > tS;
  std::set <int> eSide;
  std::set <int> fSide;
  std::set <int> :: iterator iter;
  std :: map <int , vector <int> > :: iterator iter2;
  std :: pair < set <int> , set <int> > cept;
  int src;
  int tgt;
  ceptsInPhrase.clear();

  for (int j=0; j<alignment.size(); j+=2) {
    align.push_back(stringToInteger(alignment[j+1]));
    align.push_back(stringToInteger(alignment[j]));
  }

  for (int i = 0;  i < align.size(); i+=2) {
    src = align[i];
    tgt = align[i+1];
    tS[tgt].push_back(src);
    sT[src].push_back(tgt);
  }

  for (int i = 0; i< fSize; i++) {
    if (sT.find(i) == sT.end()) {
      targetNullWords.insert(i);
    }
  }

  for (int i = 0; i< eSize; i++) {
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

}

int main(int argc, char * argv[])
{

  vector <string> e;
  vector <string> f;
  vector <string> a;
  vector <string> singletons;
  map <string,int> sTons;
  vector < pair < set <int> , set <int> > > ceptsInPhrase;
  vector < pair < string , vector <int> > > gCepts;

  set <int> sourceNullWords;
  set <int> targetNullWords;

  vector <string> currE;
  vector <string> currF;
  vector <string> currA;

  loadInput(argv[4],singletons);

  for(int i=0; i<singletons.size(); i++)
    sTons[singletons[i]]=i;

  loadInput(argv[1],e);
  loadInput(argv[2],f);
  loadInput(argv[3],a);


  for (int i=0; i<a.size(); i++) {


    getWords(e[i],currE);
    getWords(f[i],currF);
    getWords(a[i],currA);

    constructCepts(ceptsInPhrase, sourceNullWords , targetNullWords, currA , currE.size(), currF.size());
    //cout<<"CC done"<<endl;
    ceptsInGenerativeStoryFormat(ceptsInPhrase , gCepts , sourceNullWords, currE);
    //cout<<"format done"<<endl;
    // printCepts(gCepts, currF);
    generateStory(gCepts, targetNullWords ,currF,sTons);


    /*
    cout<<"________________________________________"<<endl;

    cout<<"Press any integer to continue ..."<<endl;
    int xx;
    cin>>xx;
    */

  }


  return 0;

}
