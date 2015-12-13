/*
 * AlignedSentenceSyntax.cpp
 *
 *  Created on: 26 Feb 2014
 *      Author: hieu
 */

#include "AlignedSentenceSyntax.h"
#include "Parameter.h"
#include "pugixml.hpp"
#include "moses/Util.h"

using namespace std;

AlignedSentenceSyntax::AlignedSentenceSyntax(int lineNum,
    const std::string &source,
    const std::string &target,
    const std::string &alignment)
  :AlignedSentence(lineNum)
  ,m_sourceStr(source)
  ,m_targetStr(target)
  ,m_alignmentStr(alignment)
{
}

AlignedSentenceSyntax::~AlignedSentenceSyntax()
{
  // TODO Auto-generated destructor stub
}

void AlignedSentenceSyntax::Populate(bool isSyntax, int mixedSyntaxType, const Parameter &params,
                                     string line, Phrase &phrase, SyntaxTree &tree)
{
  // parse source and target string
  if (isSyntax) {
    line = "<xml><tree label=\"X\">" + line + "</tree></xml>";
    XMLParse(phrase, tree, line, params);

    if (mixedSyntaxType != 0) {
      // mixed syntax. Always add [X] where there isn't 1
      tree.SetHieroLabel(params.hieroNonTerm);
      if (mixedSyntaxType == 2) {
        tree.AddToAll(params.hieroNonTerm);
      }
    }
  } else {
    PopulateWordVec(phrase, line);
    tree.SetHieroLabel(params.hieroNonTerm);
  }

}

void AlignedSentenceSyntax::Create(const Parameter &params)
{
  Populate(params.sourceSyntax, params.mixedSyntaxType, params, m_sourceStr,
           m_source, m_sourceTree);
  Populate(params.targetSyntax, params.mixedSyntaxType, params, m_targetStr,
           m_target, m_targetTree);

  PopulateAlignment(m_alignmentStr);
  CreateConsistentPhrases(params);

  // create labels
  CreateNonTerms();
}

void Escape(string &text)
{
  text = Moses::Replace(text, "&", "&amp;");
  text = Moses::Replace(text, "|", "&#124;");
  text = Moses::Replace(text, "<", "&lt;");
  text = Moses::Replace(text, ">", "&gt;");
  text = Moses::Replace(text, "'", "&apos;");
  text = Moses::Replace(text, "\"", "&quot;");
  text = Moses::Replace(text, "[", "&#91;");
  text = Moses::Replace(text, "]", "&#93;");

}

void AlignedSentenceSyntax::XMLParse(Phrase &output,
                                     SyntaxTree &tree,
                                     const pugi::xml_node &parentNode,
                                     const Parameter &params)
{
  int childNum = 0;
  for (pugi::xml_node childNode = parentNode.first_child(); childNode; childNode = childNode.next_sibling()) {
    string nodeName = childNode.name();

    // span label
    string label;
    int startPos = output.size();

    if (!nodeName.empty()) {
      pugi::xml_attribute attribute = childNode.attribute("label");
      label = attribute.as_string();

      // recursively call this function. For proper recursive trees
      XMLParse(output, tree, childNode, params);
    }



    // fill phrase vector
    string text = childNode.value();
    Escape(text);
    //cerr << childNum << " " << label << "=" << text << endl;

    std::vector<string> toks;
    Moses::Tokenize(toks, text);

    for (size_t i = 0; i < toks.size(); ++i) {
      const string &tok = toks[i];
      Word *word = new Word(output.size(), tok);
      output.push_back(word);
    }

    // is it a labelled span?
    int endPos = output.size() - 1;

    // fill syntax labels
    if (!label.empty()) {
      label = "[" + label + "]";
      tree.Add(startPos, endPos, label, params);
    }

    ++childNum;
  }

}

void AlignedSentenceSyntax::XMLParse(Phrase &output,
                                     SyntaxTree &tree,
                                     const std::string input,
                                     const Parameter &params)
{
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load(input.c_str(),
                                  pugi::parse_default | pugi::parse_comments);

  pugi::xml_node topNode = doc.child("xml");
  XMLParse(output, tree, topNode, params);
}

void AlignedSentenceSyntax::CreateNonTerms()
{
  for (int sourceStart = 0; sourceStart < m_source.size(); ++sourceStart) {
    for (int sourceEnd = sourceStart; sourceEnd < m_source.size(); ++sourceEnd) {
      ConsistentPhrases::Coll &coll = m_consistentPhrases.GetColl(sourceStart, sourceEnd);
      const SyntaxTree::Labels &sourceLabels = m_sourceTree.Find(sourceStart, sourceEnd);

      ConsistentPhrases::Coll::iterator iter;
      for (iter = coll.begin(); iter != coll.end(); ++iter) {
        ConsistentPhrase &cp = **iter;

        int targetStart = cp.corners[2];
        int targetEnd = cp.corners[3];
        const SyntaxTree::Labels &targetLabels = m_targetTree.Find(targetStart, targetEnd);

        CreateNonTerms(cp, sourceLabels, targetLabels);
      }
    }
  }

}

void AlignedSentenceSyntax::CreateNonTerms(ConsistentPhrase &cp,
    const SyntaxTree::Labels &sourceLabels,
    const SyntaxTree::Labels &targetLabels)
{
  SyntaxTree::Labels::const_iterator iterSource;
  for (iterSource = sourceLabels.begin(); iterSource != sourceLabels.end(); ++iterSource) {
    const string &sourceLabel = *iterSource;

    SyntaxTree::Labels::const_iterator iterTarget;
    for (iterTarget = targetLabels.begin(); iterTarget != targetLabels.end(); ++iterTarget) {
      const string &targetLabel = *iterTarget;
      cp.AddNonTerms(sourceLabel, targetLabel);
    }
  }
}


