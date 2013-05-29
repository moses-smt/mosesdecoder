// $Id$

#include <cassert>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <vector>
#include <set>
#include <algorithm>
#include <cstring>
#include "cmd.h"

using namespace std;

#define MAX_WORD 10000 // maximum lengthsource/target strings 
#define MAX_M 400      // maximum length of source strings
#define MAX_N 400      // maximum length of target strings 

#define UNION                      1
#define INTERSECT                  2
#define GROW                       3
#define SRCTOTGT                   4
#define TGTTOSRC                   5
#define BOOL_YES                   1
#define BOOL_NO                    0

#define END_ENUM    {   (char*)0,  0 }

static Enum_T AlignEnum [] = {
  {    "union",                        UNION },
  {    "u",                            UNION },
  {    "intersect",                    INTERSECT},
  {    "i",                            INTERSECT},
  {    "grow",                         GROW },
  {    "g",                            GROW },
  {    "srctotgt",                     SRCTOTGT },
  {    "s2t",                          SRCTOTGT },
  {    "tgttosrc",                     TGTTOSRC },
  {    "t2s",                          TGTTOSRC },
  END_ENUM
};

static Enum_T BoolEnum [] = {
  {    "true",        BOOL_YES },
  {    "yes",         BOOL_YES },
  {    "y",           BOOL_YES },
  {    "false",       BOOL_NO },
  {    "no",          BOOL_NO },
  {    "n",           BOOL_NO },
  END_ENUM
};



// global variables and constants

int* fa; //counters of covered foreign positions
int* ea; //counters of covered english positions
int** A; //alignment matrix with information symmetric/direct/inverse alignments

int verbose=0;

//read an alignment pair from the input stream.

int lc = 0;

int getals(fstream& inp,int& m, int *a,int& n, int *b)
{
  char w[MAX_WORD], dummy[10];
  int i,j,freq;
  if (inp >> freq) {
    ++lc;
    //target sentence
    inp >> n;
    assert(n<MAX_N);
    for (i=1; i<=n; i++) {
      inp >> setw(MAX_WORD) >> w;
      if (strlen(w)>=MAX_WORD-1) {
        cerr << lc << ": target len=" << strlen(w) << " is not less than MAX_WORD-1="
             << MAX_WORD-1 << endl;
        assert(strlen(w)<MAX_WORD-1);
      }
    }

    inp >> dummy; //# separator
    // inverse alignment
    for (i=1; i<=n; i++) inp >> b[i];

    //source sentence
    inp >> m;
    assert(m<MAX_M);
    for (j=1; j<=m; j++) {
      inp >> setw(MAX_WORD) >> w;
      if (strlen(w)>=MAX_WORD-1) {
        cerr << lc << ": source len=" << strlen(w) << " is not less than MAX_WORD-1="
             << MAX_WORD-1 << endl;
        assert(strlen(w)<MAX_WORD-1);
      }
    }

    inp >> dummy; //# separator

    // direct alignment
    for (j=1; j<=m; j++) {
      inp >> a[j];
      assert(0<=a[j] && a[j]<=n);
    }

    //check inverse alignemnt
    for (i=1; i<=n; i++)
      assert(0<=b[i] && b[i]<=m);

    return 1;

  } else
    return 0;
};


//compute union alignment
int prunionalignment(fstream& out,int m,int *a,int n,int* b)
{

  ostringstream sout;

  for (int j=1; j<=m; j++)
    if (a[j])
      sout << j-1 << "-" << a[j]-1 << " ";

  for (int i=1; i<=n; i++)
    if (b[i] && a[b[i]]!=i)
      sout << b[i]-1 <<  "-" << i-1 << " ";

  //fix the last " "
  string str = sout.str();
  if (str.length() == 0)
    str = "\n";
  else
    str.replace(str.length()-1,1,"\n");

  out << str;
  out.flush();

  return 1;
}


//Compute intersection alignment

int printersect(fstream& out,int m,int *a,int n,int* b)
{

  ostringstream sout;

  for (int j=1; j<=m; j++)
    if (a[j] && b[a[j]]==j)
      sout << j-1 << "-" << a[j]-1 << " ";

  //fix the last " "
  string str = sout.str();
  if (str.length() == 0)
    str = "\n";
  else
    str.replace(str.length()-1,1,"\n");

  out << str;
  out.flush();

  return 1;
}

//Compute target-to-source alignment

int printtgttosrc(fstream& out,int m,int *a,int n,int* b)
{

  ostringstream sout;

  for (int i=1; i<=n; i++)
    if (b[i])
      sout << b[i]-1 << "-" << i-1 << " ";

  //fix the last " "
  string str = sout.str();
  if (str.length() == 0)
    str = "\n";
  else
    str.replace(str.length()-1,1,"\n");

  out << str;
  out.flush();

  return 1;
}

//Compute source-to-target alignment

int printsrctotgt(fstream& out,int m,int *a,int n,int* b)
{

  ostringstream sout;

  for (int j=1; j<=m; j++)
    if (a[j])
      sout << j-1 << "-" << a[j]-1 << " ";

  //fix the last " "
  string str = sout.str();
  if (str.length() == 0)
    str = "\n";
  else
    str.replace(str.length()-1,1,"\n");

  out << str;
  out.flush();

  return 1;
}

//Compute Grow Diagonal Alignment
//Nice property: you will never introduce more points
//than the unionalignment alignemt. Hence, you will always be able
//to represent the grow alignment as the unionalignment of a
//directed and inverted alignment

int printgrow(fstream& out,int m,int *a,int n,int* b, bool diagonal=false,bool final=false,bool bothuncovered=false)
{

  ostringstream sout;

  vector <pair <int,int> > neighbors; //neighbors

  pair <int,int> entry;

  neighbors.push_back(make_pair(-1,-0));
  neighbors.push_back(make_pair(0,-1));
  neighbors.push_back(make_pair(1,0));
  neighbors.push_back(make_pair(0,1));


  if (diagonal) {
    neighbors.push_back(make_pair(-1,-1));
    neighbors.push_back(make_pair(-1,1));
    neighbors.push_back(make_pair(1,-1));
    neighbors.push_back(make_pair(1,1));
  }


  int i,j;
  size_t o;


  //covered foreign and english positions

  memset(fa,0,(m+1)*sizeof(int));
  memset(ea,0,(n+1)*sizeof(int));

  //matrix to quickly check if one point is in the symmetric
  //alignment (value=2), direct alignment (=1) and inverse alignment

  for (int i=1; i<=n; i++) memset(A[i],0,(m+1)*sizeof(int));

  set <pair <int,int> > currentpoints; //symmetric alignment
  set <pair <int,int> > unionalignment; //union alignment

  pair <int,int> point; //variable to store points
  set<pair <int,int> >::const_iterator k; //iterator over sets

  //fill in the alignments
  for (j=1; j<=m; j++) {
    if (a[j]) {
      unionalignment.insert(make_pair(a[j],j));
      if (b[a[j]]==j) {
        fa[j]=1;
        ea[a[j]]=1;
        A[a[j]][j]=2;
        currentpoints.insert(make_pair(a[j],j));
      } else
        A[a[j]][j]=-1;
    }
  }

  for (i=1; i<=n; i++)
    if (b[i] && a[b[i]]!=i) { //not intersection
      unionalignment.insert(make_pair(i,b[i]));
      A[i][b[i]]=1;
    }


  int added=1;

  while (added) {
    added=0;
    ///scan the current alignment
    for (k=currentpoints.begin(); k!=currentpoints.end(); k++) {
      //cout << "{"<< (k->second)-1 << "-" << (k->first)-1 << "}";
      for (o=0; o<neighbors.size(); o++) {
        //cout << "go over check all neighbors\n";
        point.first=k->first+neighbors[o].first;
        point.second=k->second+neighbors[o].second;
        //cout << point.second-1 << " " << point.first-1 << "\n";
        //check if neighbor is inside 'matrix'
        if (point.first>0 && point.first <=n && point.second>0 && point.second<=m)
          //check if neighbor is in the unionalignment alignment
          if (b[point.first]==point.second || a[point.second]==point.first) {
            //cout << "In unionalignment ";cout.flush();
            //check if it connects at least one uncovered word
            if (!(ea[point.first] && fa[point.second])) {
              //insert point in currentpoints!
              currentpoints.insert(point);
              A[point.first][point.second]=2;
              ea[point.first]=1;
              fa[point.second]=1;
              added=1;
              //cout << "added grow: " << point.second-1 << "-" << point.first-1 << "\n";cout.flush();
            }
          }
      }
    }
  }

  if (final) {
    for (k=unionalignment.begin(); k!=unionalignment.end(); k++)
      if (A[k->first][k->second]==1) {
        point.first=k->first;
        point.second=k->second;
        //one of the two words is not covered yet
        //cout << "{" << point.second-1 << "-" << point.first-1 << "} ";
        if ((bothuncovered &&  !ea[point.first] && !fa[point.second]) ||
            (!bothuncovered && !(ea[point.first] && fa[point.second]))) {
          //add it!
          currentpoints.insert(point);
          A[point.first][point.second]=2;
          //keep track of new covered positions
          ea[point.first]=1;
          fa[point.second]=1;

          //added=1;
          //cout << "added final: " << point.second-1 << "-" << point.first-1 << "\n";
        }
      }

    for (k=unionalignment.begin(); k!=unionalignment.end(); k++)
      if (A[k->first][k->second]==-1) {
        point.first=k->first;
        point.second=k->second;
        //one of the two words is not covered yet
        //cout << "{" << point.second-1 << "-" << point.first-1 << "} ";
        if ((bothuncovered &&  !ea[point.first] && !fa[point.second]) ||
            (!bothuncovered && !(ea[point.first] && fa[point.second]))) {
          //add it!
          currentpoints.insert(point);
          A[point.first][point.second]=2;
          //keep track of new covered positions
          ea[point.first]=1;
          fa[point.second]=1;

          //added=1;
          //cout << "added final: " << point.second-1 << "-" << point.first-1 << "\n";
        }
      }
  }


  for (k=currentpoints.begin(); k!=currentpoints.end(); k++)
    sout << k->second-1 << "-" << k->first-1 << " ";


  //fix the last " "
  string str = sout.str();
  if (str.length() == 0)
    str = "\n";
  else
    str.replace(str.length()-1,1,"\n");

  out << str;
  out.flush();
  return 1;

  return 1;
}



//Main file here


int main(int argc, char** argv)
{

  int alignment=0;
  char* input=(char*)"/dev/stdin";
  char* output=(char*)"/dev/stdout";
  int diagonal=false;
  int final=false;
  int bothuncovered=false;


  DeclareParams("a", CMDENUMTYPE,  &alignment, AlignEnum,
                "alignment", CMDENUMTYPE,  &alignment, AlignEnum,
                "d", CMDENUMTYPE,  &diagonal, BoolEnum,
                "diagonal", CMDENUMTYPE,  &diagonal, BoolEnum,
                "f", CMDENUMTYPE,  &final, BoolEnum,
                "final", CMDENUMTYPE,  &final, BoolEnum,
                "b", CMDENUMTYPE,  &bothuncovered, BoolEnum,
                "both", CMDENUMTYPE,  &bothuncovered, BoolEnum,
                "i", CMDSTRINGTYPE, &input,
                "o", CMDSTRINGTYPE, &output,
                "v", CMDENUMTYPE,  &verbose, BoolEnum,
                "verbose", CMDENUMTYPE,  &verbose, BoolEnum,

                (char*)NULL);

  GetParams(&argc, &argv, (char*)NULL);

  if (alignment==0) {
    cerr << "usage: symal [-i=<inputfile>] [-o=<outputfile>] -a=[u|i|g] -d=[yes|no] -b=[yes|no] -f=[yes|no] \n"
         << "Input file or std must be in .bal format (see script giza2bal.pl).\n";

    exit(1);

  }

  fstream inp(input,ios::in);
  fstream out(output,ios::out);

  if (!inp.is_open()) {
    cerr << "cannot open " << input << "\n";
    exit(1);
  }

  if (!out.is_open()) {
    cerr << "cannot open " << output << "\n";
    exit(1);
  }


  int a[MAX_M],b[MAX_N],m,n;
  fa=new int[MAX_M+1];
  ea=new int[MAX_N+1];


  int sents = 0;
  A=new int *[MAX_N+1];
  for (int i=1; i<=MAX_N; i++) A[i]=new int[MAX_M+1];

  switch (alignment) {
  case UNION:
    cerr << "symal: computing union alignment\n";
    while(getals(inp,m,a,n,b)) {
      prunionalignment(out,m,a,n,b);
      sents++;
    }
    cerr << "Sents: " << sents << endl;
    break;
  case INTERSECT:
    cerr << "symal: computing intersect alignment\n";
    while(getals(inp,m,a,n,b)) {
      printersect(out,m,a,n,b);
      sents++;
    }
    cerr << "Sents: " << sents << endl;
    break;
  case GROW:
    cerr << "symal: computing grow alignment: diagonal ("
         << diagonal << ") final ("<< final << ")"
         <<  "both-uncovered (" << bothuncovered <<")\n";

    while(getals(inp,m,a,n,b))
      printgrow(out,m,a,n,b,diagonal,final,bothuncovered);

    break;
  case TGTTOSRC:
    cerr << "symal: computing target-to-source alignment\n";

    while(getals(inp,m,a,n,b)) {
      printtgttosrc(out,m,a,n,b);
      sents++;
    }
    cerr << "Sents: " << sents << endl;
    break;
  case SRCTOTGT:
    cerr << "symal: computing source-to-target alignment\n";

    while(getals(inp,m,a,n,b)) {
      printsrctotgt(out,m,a,n,b);
      sents++;
    }
    cerr << "Sents: " << sents << endl;
    break;
  default:
    exit(1);
  }

  delete [] fa;
  delete [] ea;
  for (int i=1; i<=MAX_N; i++) delete [] A[i];
  delete [] A;

  exit(0);
}
