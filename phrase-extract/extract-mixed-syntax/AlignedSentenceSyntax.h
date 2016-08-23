/*
 * AlignedSentenceSyntax.h
 *
 *  Created on: 26 Feb 2014
 *      Author: hieu
 */

#pragma once

#include "AlignedSentence.h"
#include "SyntaxTree.h"
#include "pugixml.hpp"

class AlignedSentenceSyntax : public AlignedSentence
{
public:
  AlignedSentenceSyntax(int lineNum,
                        const std::string &source,
                        const std::string &target,
                        const std::string &alignment);
  virtual ~AlignedSentenceSyntax();

  void Create(const Parameter &params);

  //virtual std::string Debug() const;
protected:
  std::string m_sourceStr, m_targetStr, m_alignmentStr;
  SyntaxTree m_sourceTree, m_targetTree;

  void XMLParse(Phrase &output,
                SyntaxTree &tree,
                const std::string input,
                const Parameter &params);
  void XMLParse(Phrase &output,
                SyntaxTree &tree,
                const pugi::xml_node &parentNode,
                const Parameter &params);
  void CreateNonTerms();
  void CreateNonTerms(ConsistentPhrase &cp,
                      const SyntaxTree::Labels &sourceLabels,
                      const SyntaxTree::Labels &targetLabels);
  void Populate(bool isSyntax, int mixedSyntaxType, const Parameter &params,
                std::string line, Phrase &phrase, SyntaxTree &tree);

};

