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
  vector <string> mAlign;

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
  int res;

  for (int j=0; j<alignment.size(); j+=1) {
    res = alignment[j].find("-");
    mAlign.push_back(alignment[j].substr(0,res));
    mAlign.push_back(alignment[j].substr(res+1));
  }

  for (int j=0; j<mAlign.size(); j+=2) {
    align.push_back(stringToInteger(mAlign[j+1]));
    align.push_back(stringToInteger(mAlign[j]));
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

void getOneToOne(vector < pair < set <int> , set <int> > > & ceptsInPhrase , vector <string> & currF , vector <string> & currE, set <string> & one)
{
  string temp;

  for (int i = 0; i< ceptsInPhrase.size(); i++) {
    if (ceptsInPhrase[i].first.size() == 1 && ceptsInPhrase[i].second.size() == 1) {
      temp = currF[(*ceptsInPhrase[i].second.begin())] + "\t" + currE[(*ceptsInPhrase[i].first.begin())];

      if (one.find(temp) == one.end())
        one.insert(temp);
    }
  }

}

void printOneToOne ( set <string> & one)
{
  set <string> :: iterator iter;

  for (iter = one.begin(); iter != one.end(); iter++) {
    cout<<*iter<<endl;
  }
}

int main(int argc, char * argv[])
{

  vector <string> e;
  vector <string> f;
  vector <string> a;
  vector < pair < set <int> , set <int> > > ceptsInPhrase;
  vector < pair < string , vector <int> > > gCepts;

  set <int> sourceNullWords;
  set <int> targetNullWords;

  vector <string> currE;
  vector <string> currF;
  vector <string> currA;
  set <string> one;

  loadInput(argv[1],f);
  loadInput(argv[2],e);
  loadInput(argv[3],a);


  for (int i=0; i<a.size(); i++) {


    getWords(e[i],currE);
    getWords(f[i],currF);
    getWords(a[i],currA);

    if (i % 100000 == 0) {
      cerr<<"Processing "<<i<<endl;
    }
    constructCepts(ceptsInPhrase, sourceNullWords , targetNullWords, currA , currE.size(), currF.size());
    getOneToOne(ceptsInPhrase , currF , currE, one);

    /*
    cout<<"________________________________________"<<endl;

    cout<<"Press any integer to continue ..."<<endl;
    int xx;
    cin>>xx;
    */

  }

  printOneToOne(one);


  return 0;

}
