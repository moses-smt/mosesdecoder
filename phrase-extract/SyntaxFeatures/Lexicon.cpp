#include <vector>
#include <string>
#include <fstream>
#include <cassert>

#include "Lexicon.h"
using namespace std;

void Lexicon::insert_in_order(string word){
  lexicon.push_back(word);
}

string Lexicon::get_word(int position) const{
  assert(position < lexicon.size());
  return lexicon[position];
}

void Lexicon::readin(istream &in){
  while (! in.eof()){
    string line;
    getline(in,line);
    if (line != ""){
      StringUtils s;
      vector<string> entries;
      s.SplitString(line,"\t",entries);
      assert(entries.size() == 2);
      lexicon.push_back(entries[1]);
    }
  }
}

void Lexicon::printout(ostream &out) const{
  for(int i = 0; i < lexicon.size(); i++){
    out << i << "\t" << lexicon[i] << endl;
  }
}

int Lexicon::binary_search(string word,int begin, int end) const{
  if (end - begin == 0)
    return -1;
  if (lexicon[begin] == word)
    return begin;
  if (lexicon[end] == word)
    return end;
  if (end - begin < 2)
    return -1;
  int middle = begin + (end - begin)/2;
  if ( word < lexicon[middle] ){
    return binary_search(word,begin,middle);
  }else{
    return binary_search(word,middle,end);
  }
}

bool Lexicon::exists(string word) const{
  int i = binary_search(word,0,lexicon.size() - 1);
  assert ( i >= -1);
  if ( i == -1 )
    return false;
  else if ( i >= 0)
    return true;
}

int Lexicon::get_id(string word) const{
  int i = binary_search(word,0,lexicon.size() - 1);
  assert ( i >= -1);
  return i;
}

/*
void Lexicon::insert(string word){
  if (word > lexicon[lexicon.size() - 1]){
    lexicon.push_back(word);
  }else{
  }
}*/
