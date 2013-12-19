/* File: javaforcealigner.i */
%include "std_string.i"

%module javaforcealigner
%{
#include "ForceAligner.h"
#include "SymForceAligner.h"
%}

class Corpus {
  public:
    Corpus();
    void addSentence(std::string e, std::string f);
};

class ForceAligner {
  public:
    ForceAligner(std::string src, std::string trg, std::string path);
    
    std::string alignCorpusStr(Corpus& corpus);
    std::string alignSentenceStr(std::string e, std::string f);
    bool errorOccurred();
};

class SymForceAligner {
  public:
    enum Mode { Src2Trg, Trg2Src, Intersection, Union, Grow, GrowDiag, GrowDiagFinal, GrowDiagFinalAnd };
    SymForceAligner(std::string src, std::string trg, std::string path);
    void setMode(Mode mode);
    
    std::string alignCorpusStr(Corpus& corpus);
    std::string alignSentenceStr(std::string e, std::string f);
    bool errorOccurred();
};
