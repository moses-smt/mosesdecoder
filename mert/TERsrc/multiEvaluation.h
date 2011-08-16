#ifndef __MULTIEVAL_DOCUMENT_H__
#define __MULTIEVAL_DOCUMENT_H__

#include "multiTxtDocument.h"
#include "tools.h"
#include <iostream>
#include <string>
#include "xmlStructure.h"
#include "sgmlDocument.h"

using namespace Tools;
namespace TERCpp
{

class multiEvaluation
{
public:
  multiEvaluation();
  multiEvaluation(param p );
//     void addReferences(string s);
//     void addReferences(vector<string> vecRefecrences);
//     void addReferences(documentStructure doc);
//     void setHypothesis(string s);
//     void setHypothesis(documentStructure doc);
  void addReferences();
  void setHypothesis();
  void addSGMLReferences();
  void setSGMLHypothesis();
  void setParameters ( param p );
  void launchTxtEvaluation();
  void launchSGMLEvaluation();
  void evaluate ( documentStructure & docStructReference, documentStructure & docStructhypothesis );
  string scoreTER ( vector<float> numEdits, vector<float> numWords );
private:
  param evalParameters;
  multiTxtDocument referencesTxt;
  documentStructure hypothesisTxt;
  SGMLDocument referencesSGML;
  documentStructure hypothesisSGML;


};
}
#endif //SANDWICH_DEFINED
