//

#ifndef moses_SyntacticLanguageModelFiles_h
#define moses_SyntacticLanguageModelFiles_h

#include "nl-iomacros.h"
#include "nl-string.h"

namespace Moses
{

template <class MH, class MO>
class SyntacticLanguageModelFiles
{

public:

  SyntacticLanguageModelFiles(const std::vector<std::string>& filePaths);
  ~SyntacticLanguageModelFiles();

  MH* getHiddenModel();
  MO* getObservedModel();

private:
  MH* hiddenModel;
  MO* observedModel;

};


template <class MH, class MO>
SyntacticLanguageModelFiles<MH,MO>::SyntacticLanguageModelFiles(const std::vector<std::string>& filePaths)
{

  this->hiddenModel = new MH();
  this->observedModel = new MO();

  //// I. LOAD MODELS...
  std::cerr << "Reading syntactic language model files...\n";
  // For each model file...
  for ( int a=0, n=filePaths.size(); a<n; a++ ) {                                           // read models
    FILE* pf = fopen(filePaths[a].c_str(),"r"); // Read model file
    if(!pf) {
      std::cerr << "Error loading model file " << filePaths[a] << std::endl;
      return;
    }
    std::cerr << "Loading model \'" << filePaths[a] << "\'...\n";
    int c=' ';
    int i=0;
    int line=1;
    String sBuff(1000);                          // Lookahead/ctrs/buffers
    CONSUME_ALL ( pf, c, WHITESPACE(c), line);                                   // Get to first record
    while ( c!=-1 && c!='\0' && c!='\5' ) {                                      // For each record
      CONSUME_STR ( pf, c, (c!='\n' && c!='\0' && c!='\5'), sBuff, i, line );    //   Consume line
      StringInput si(sBuff.c_array());
      if ( !( sBuff[0]=='#'                                                   //   Accept comments/fields
              ||  si>>*(this->hiddenModel)>>"\0"!=NULL
              ||  si>>*(this->observedModel)>>"\0"!=NULL
            ))
        std::cerr<<"\nERROR: can't parse \'"<<sBuff<<"\' in line "<<line<<"\n\n";
      CONSUME_ALL ( pf, c, WHITESPACE(c), line);                                 //   Consume whitespace
      if ( line%100000==0 ) std::cerr<<"  "<<line<<" lines read...\n";                //   Progress for big models
    }
    std::cerr << "Model \'" << filePaths[a] << "\' loaded.\n";
  }

  std::cerr << "...reading syntactic language model files completed\n";


}


template <class MH, class MO>
SyntacticLanguageModelFiles<MH,MO>::~SyntacticLanguageModelFiles()
{

  VERBOSE(3,"Destructing syntactic language model files" << std::endl);
  delete hiddenModel;
  delete observedModel;

}


template <class MH, class MO>
MH* SyntacticLanguageModelFiles<MH,MO>::getHiddenModel()
{

  return this->hiddenModel;

}

template <class MH, class MO>
MO* SyntacticLanguageModelFiles<MH,MO>::getObservedModel()
{

  return this->observedModel;

}


}

#endif
