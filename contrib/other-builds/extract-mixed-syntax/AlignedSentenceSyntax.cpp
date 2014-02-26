/*
 * AlignedSentenceSyntax.cpp
 *
 *  Created on: 26 Feb 2014
 *      Author: hieu
 */

#include "AlignedSentenceSyntax.h"
#include "pugixml.hpp"

AlignedSentenceSyntax::AlignedSentenceSyntax(const std::string &source,
		const std::string &target,
		const std::string &alignment)
:AlignedSentence(source, target, alignment)
{
	// TODO Auto-generated constructor stub

}

AlignedSentenceSyntax::~AlignedSentenceSyntax() {
	// TODO Auto-generated destructor stub
}

void AlignedSentenceSyntax::CreateConsistentPhrases(const Parameter &params)
{
	pugi::xml_document doc;

	pugi::xml_parse_result result = doc.load_file("tree.xml");
}
