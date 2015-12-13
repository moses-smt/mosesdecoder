
#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>
#include <string>
#include "moses/Util.h"
#include "Alignments.h"

using namespace std;
using namespace Moses;

inline const std::string TrimInternal(const std::string& str, const std::string dropChars = " \t\n\r")
{
  std::string res = str;
  res.erase(str.find_last_not_of(dropChars)+1);
  return res.erase(0, res.find_first_not_of(dropChars));
}

class CreateXMLRetValues
{
public:
  string frame, ruleS, ruleT, ruleAlignment, ruleAlignmentInv;
};

CreateXMLRetValues createXML(int ruleCount, const string &source, const string &input, const string &target, const string &align, const string &path );

void create_xml(const string &inPath)
{
  ifstream inStrme(inPath.c_str());
  ofstream rule((inPath + ".extract").c_str());
  ofstream ruleInv((inPath + ".extract.inv").c_str());

  // int setenceId;
  // float score;
  string source, target, align, path;
  string *input = NULL;
  int count;

  int lineCount = 1;
  int ruleCount = 1;
  string inLine;

  int step = 0;
  while (!inStrme.eof()) {
    getline(inStrme, inLine);
    //cout << inLine << endl;
    switch (step) {
    case 0:
      /*setenceId = */
      Scan<int>(inLine);
      ++step;
      break;
    case 1:
      /*score = */
      Scan<float>(inLine);
      ++step;
      break;
    case 2:
      source = inLine;
      ++step;
      break;
    case 3:
      if (input == NULL) {
        input = new string(inLine);
      } else {
        assert(inLine == *input);
      }
      ++step;
      break;
    case 4:
      target = inLine;
      ++step;
      break;
    case 5:
      align = inLine;
      ++step;
      break;
    case 6:
      path = inLine + "X";
      ++step;
      break;
    case 7:
      count = Scan<int>(inLine);
      CreateXMLRetValues ret = createXML(ruleCount, source, *input, target, align, path);

      //print STDOUT $frame."\n";
      rule << ret.ruleS << " [X] ||| " << ret.ruleT << " [X] ||| " << ret.ruleAlignment
           << " ||| " << count << endl;
      ruleInv << ret.ruleT << " [X] ||| " << ret.ruleS << " [X] ||| " << ret.ruleAlignmentInv
              << " ||| " << count << endl;

      //print STDOUT "$sentenceInd ||| $score ||| $count\n";
      ++ruleCount;
      step = 0;
      break;
    }

    ++lineCount;
  }

  delete input;
  ruleInv.close();
  rule.close();
  inStrme.close();

}


CreateXMLRetValues createXML(int ruleCount, const string &source, const string &input, const string &target, const string &align, const string &path)
{
  CreateXMLRetValues ret;
  vector<string> sourceToks   = Tokenize(source, " ")
                                ,inputToks    = Tokenize(input, " ")
                                    ,targetsToks  = Tokenize(target, " ");
  Alignments alignments(align, sourceToks.size(), targetsToks.size());
  map<int, string> frameInput;
  map<int, int> alignI2S;
  vector< map<string, int> > nonTerms;
  vector<bool> targetBitmap(targetsToks.size(), true);
  vector<bool> inputBitmap;

  // STEP 1: FIND MISMATCHES
  int s = 0, i = 0;
  bool currently_matching = false;
  int start_s = 0, start_i = 0;

  //cerr << input << endl << source << endl << target << endl << path << endl;
  for ( int p = 0 ; p < int(path.length()) ; p++ ) {
    string action = path.substr(p, 1);

    // beginning of a mismatch
    if ( currently_matching && action != "M" && action != "X" ) {
      start_i            = i;
      start_s            = s;
      currently_matching = 0;
    } // if ( currently_matching
    // end of a mismatch
    else if ( !currently_matching && ( action == "M" || action == "X" ) ) {

      // remove use of affected target words
      for ( int ss = start_s ; ss < s ; ss++ ) {
        const std::map<int, int> &targets = alignments.m_alignS2T[ss];

        std::map<int, int>::const_iterator iter;
        for (iter = targets.begin(); iter != targets.end(); ++iter) {
          int tt = iter->first;
          targetBitmap[tt] = 0;
        }

        // also remove enclosed unaligned words?
      } //for ( int ss = start_s ; ss < s ; ss++ ) {

      // are there input words that need to be inserted ?
      //cerr << start_i << "<" << i << "?" << endl;
      if (start_i < i ) {

        // take note of input words to be inserted
        string insertion = "";
        for (int ii = start_i ; ii < i ; ii++ ) {
          insertion += inputToks[ii] + " ";
        }

        // find position for inserted input words

        // find first removed target word
        int start_t = 1000;
        for ( int ss = start_s ; ss < s ; ss++ ) {
          const std::map<int, int> &targets = alignments.m_alignS2T[ss];

          std::map<int, int>::const_iterator iter;
          for (iter = targets.begin(); iter != targets.end(); ++iter) {
            int tt = iter->first;
            if (tt < start_t) {
              start_t = tt;
            }
          }
        }

        // end of sentence? add to end
        if ( start_t == 1000 && i > int(inputToks.size()) - 1 ) {
          start_t = targetsToks.size() - 1;
        }

        // backtrack to previous words if unaligned
        if ( start_t == 1000 ) {
          start_t = -1;
          for ( int ss = s - 1 ; start_t == -1 && ss >= 0 ; ss-- ) {
            const std::map<int, int> &targets = alignments.m_alignS2T[ss];

            std::map<int, int>::const_iterator iter;
            for (iter = targets.begin(); iter != targets.end(); ++iter) {
              int tt = iter->first;
              if (tt > start_t) {
                start_t = tt;
              }
            }
          }
        } // if ( start_t == 1000 ) {

        frameInput[start_t] += insertion;
        map<string, int> nt;
        nt["start_t"] = start_t;
        nt["start_i"] = start_i;
        nonTerms.push_back(nt);

      } // if (start_i < i ) {

      currently_matching = 1;
    } // else if ( !currently_matching

    /*
    cerr << action << " " << s << " " << i
    		<< "(" << start_s << " " << start_i << ")"
    		<< currently_matching;
     */

    if ( action != "I" ) {
      //cerr << " ->";

      if (s < int(alignments.m_alignS2T.size())) {
        const std::map<int, int> &targets = alignments.m_alignS2T[s];
        //cerr << "s=" << s << endl;

        std::map<int, int>::const_iterator iter;
        for (iter = targets.begin(); iter != targets.end(); ++iter) {
          // int tt = iter->first;
          //cerr << " " << tt;
        }
      }
    }
    //cerr << endl;

    if (action != "I")
      s++;
    if (action != "D") {
      i++;
      alignI2S[i] = s;
    }

    if (action == "M") {
      inputBitmap.push_back(1);
    } else if (action == "I" || action == "S") {
      inputBitmap.push_back(0);
    }

  } // for ( int p = 0

  //cerr << target << endl;
  for (size_t i = 0; i < targetBitmap.size(); ++i) {
    //cerr << targetBitmap[i];
  }
  //cerr << endl;

  for (map<int, string>::const_iterator iter = frameInput.begin(); iter != frameInput.end(); ++iter) {
    //cerr << iter->first << ":" <<iter->second << endl;
  }

  // STEP 2: BUILD RULE AND FRAME

  // hierarchical rule
  int rule_pos_s = 0;
  map<int, int> ruleAlignS;

  for (int i = 0 ; i < int(inputBitmap.size()) ; ++i ) {
    if ( inputBitmap[i] ) {
      ret.ruleS += inputToks[i] + " ";
      ruleAlignS[ alignI2S[i] ] = rule_pos_s++;
    }

    for (size_t j = 0; j < nonTerms.size(); ++j) {
      map<string, int> &nt = nonTerms[j];
      if (i == nt["start_i"]) {
        ret.ruleS += "[X][X] ";
        nt["rule_pos_s"] = rule_pos_s++;
      }
    }
  }

  int rule_pos_t = 0;
  map<int, int> ruleAlignT;

  for (int t = -1 ; t < (int) targetBitmap.size(); t++ ) {
    if (t >= 0 && targetBitmap[t]) {
      ret.ruleT += targetsToks[t] + " ";
      ruleAlignT[t] = rule_pos_t++;
    }

    for (size_t i = 0; i < nonTerms.size(); ++i) {
      map<string, int> &nt = nonTerms[i];

      if (t == nt["start_t"]) {
        ret.ruleT += "[X][X] ";
        nt["rule_pos_t"] = rule_pos_t++;
      }
    }
  }

  int numAlign = 0;
  ret.ruleAlignment = "";

  for (map<int, int>::const_iterator iter = ruleAlignS.begin(); iter != ruleAlignS.end(); ++iter) {
    int s = iter->first;

    if (s < int(alignments.m_alignS2T.size())) {
      const std::map<int, int> &targets = alignments.m_alignS2T[s];

      std::map<int, int>::const_iterator iter;
      for (iter = targets.begin(); iter != targets.end(); ++iter) {
        int t =iter->first;
        if (ruleAlignT.find(t) == ruleAlignT.end())
          continue;
        ret.ruleAlignment += SPrint(ruleAlignS[s]) + "-" + SPrint(ruleAlignT[t]) + " ";
        ++numAlign;
      }
    }
  }

  //cerr << "numAlign=" << numAlign << endl;

  for (size_t i = 0; i < nonTerms.size(); ++i) {
    map<string, int> &nt = nonTerms[i];
    ret.ruleAlignment += SPrint(nt["rule_pos_s"]) + "-" + SPrint(nt["rule_pos_t"]) + " ";
    ++numAlign;
  }

  //cerr << "numAlign=" << numAlign << endl;

  ret.ruleS = TrimInternal(ret.ruleS);
  ret.ruleT = TrimInternal(ret.ruleT);
  ret.ruleAlignment = TrimInternal(ret.ruleAlignment);

  vector<string> ruleAlignmentToks = Tokenize(ret.ruleAlignment);
  for (size_t i = 0; i < ruleAlignmentToks.size(); ++i) {
    const string &alignPoint = ruleAlignmentToks[i];
    vector<string> toks = Tokenize(alignPoint, "-");
    assert(toks.size() == 2);
    ret.ruleAlignmentInv += toks[1] + "-" +toks[0];
  }
  ret.ruleAlignmentInv = TrimInternal(ret.ruleAlignmentInv);

  // frame
  // ret.frame;
  if (frameInput.find(-1) == frameInput.end())
    ret.frame = frameInput[-1];

  int currently_included = 0;
  int start_t            = -1;
  targetBitmap.push_back(0);

  for (size_t t = 0 ; t <= targetsToks.size() ; t++ ) {
    // beginning of tm target inclusion
    if ( !currently_included && targetBitmap[t] ) {
      start_t            = t;
      currently_included = 1;
    }
    // end of tm target inclusion (not included word or inserted input)
    else if (currently_included
             && ( targetBitmap[t] || frameInput.find(t) != frameInput.end() )
            ) {
      // add xml (unless change is at the beginning of the sentence
      if ( start_t >= 0 ) {
        string target = "";
        //cerr << "for(tt=$start_t;tt<$t+$TARGET_BITMAP[$t]);\n";
        for (size_t tt = start_t ; tt < t + targetBitmap[t] ; tt++ ) {
          target += targetsToks[tt] + " ";
        }
        // target = Trim(target); TODO
        ret.frame += "<xml translation=\"" + target + "\"> x </xml> ";
      }
      currently_included = 0;
    }

    if (frameInput.find(t) != frameInput.end())
      ret.frame += frameInput[t];
    //cerr << targetBitmap[t] << " " << t << " " << "(" << start_t << ")"
    //			<< currently_included << endl;

  } //for (int t = 0

  cerr << ret.frame << "\n-------------------------------------\n";
  return ret;

}



