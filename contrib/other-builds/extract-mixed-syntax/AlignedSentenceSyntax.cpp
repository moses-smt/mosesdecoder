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
:AlignedSentence()
,m_source(source)
,m_target(target)
,m_alignment(alignment)
{
	// TODO Auto-generated constructor stub

}

AlignedSentenceSyntax::~AlignedSentenceSyntax() {
	// TODO Auto-generated destructor stub
}

void AlignedSentenceSyntax::CreateConsistentPhrases(const Parameter &params)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load("<node id='123'>text</node><!-- comment -->", pugi::parse_default | pugi::parse_comments);

	pugi::xml_node node = doc.child("node");

	// change node name
	std::cout << node.set_name("notnode");
	std::cout << ", new node name: " << node.name() << std::endl;

	// change comment text
	std::cout << doc.last_child().set_value("useless comment");
	std::cout << ", new comment text: " << doc.last_child().value() << std::endl;

	// we can't change value of the element or name of the comment
	std::cout << node.set_value("1") << ", " << doc.last_child().set_name("2") << std::endl;
	//]

	//[code_modify_base_attr
	pugi::xml_attribute attr = node.attribute("id");

	// change attribute name/value
	std::cout << attr.set_name("key") << ", " << attr.set_value("345");
	std::cout << ", new attribute: " << attr.name() << "=" << attr.value() << std::endl;

	// we can use numbers or booleans
	attr.set_value(1.234);
	std::cout << "new attribute value: " << attr.value() << std::endl;

	// we can also use assignment operators for more concise code
	attr = true;
	std::cout << "final attribute value: " << attr.value() << std::endl;
}
