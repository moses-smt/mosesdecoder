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

AlignedSentenceSyntax::AlignedSentenceSyntax(const std::string &source,
		const std::string &target,
		const std::string &alignment)
:AlignedSentence()
,m_sourceStr(source)
,m_targetStr(target)
,m_alignmentStr(alignment)
{
	cerr << "syntax" << endl;

}

AlignedSentenceSyntax::~AlignedSentenceSyntax() {
	// TODO Auto-generated destructor stub
}

void AlignedSentenceSyntax::Create(const Parameter &params)
{
	// parse source and target string
	if (params.sourceSyntax) {
		m_sourceStr = "<xml>" + m_sourceStr + "</xml>";
		XMLParse(m_source, m_sourceTree, m_sourceStr, params);
	}
	else {
		PopulateWordVec(m_source, m_sourceStr);
	}

	if (params.targetSyntax) {
		m_targetStr = "<xml>" + m_targetStr + "</xml>";
		XMLParse(m_target, m_targetTree, m_targetStr, params);
	}
	else {
		PopulateWordVec(m_target, m_targetStr);
	}

	PopulateAlignment(m_alignmentStr);
	CreateConsistentPhrases(params);

	// create labels
	CreateNonTerms();
}

void AlignedSentenceSyntax::XMLParse(Phrase &output, SyntaxTree &tree, const std::string input, const Parameter &params)
{
	cerr << "input=" << input << endl;

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load(input.c_str(), pugi::parse_default | pugi::parse_comments);

	pugi::xml_node topNode = doc.child("xml");
	std::cerr << topNode.name() << std::endl;

    for (pugi::xml_node node = topNode.first_child(); node; node = node.next_sibling())
    {
        std::cerr << node.name() << std::endl;

        for (pugi::xml_attribute attr = node.first_attribute(); attr; attr = attr.next_attribute())
        {
            std::cerr << " " << attr.name() << "=" << attr.value() << std::endl;
        }
        std::cerr << node.text().as_string() << std::endl;

        // fill data structures
        int startPos = output.size();

        // fill phrase vector
    	string text = node.text().as_string();

    	std::vector<string> toks;
    	Moses::Tokenize(toks, text);

    	for (size_t i = 0; i < toks.size(); ++i) {
    		const string &tok = toks[i];
    		Word *word = new Word(i, tok);
    		output.push_back(word);
    	}
    	int endPos = output.size() - 1;

    	// fill syntax labels
        string nodeName = node.name();

        if (!nodeName.empty()) {
        	tree.Add(startPos, endPos, nodeName);
        }
    }
    std::cerr << std::endl;
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
				const SyntaxTree::Labels &targetLabels = m_sourceTree.Find(targetStart, targetEnd);

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
		SyntaxTree::Labels::const_iterator iterTarget;
		const string &sourceLabel = *iterSource;

		for (iterTarget = targetLabels.begin(); iterTarget != targetLabels.end(); ++iterTarget) {
			const string &targetLabel = *iterTarget;
			cp.AddNonTerms(sourceLabel, targetLabel);
		}
	}
}


