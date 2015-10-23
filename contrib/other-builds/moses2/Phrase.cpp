/*
 * Phrase.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include "Phrase.h"
#include "Word.h"

Phrase::Phrase(size_t size)
{
  m_words = new Word[size];

}

Phrase::~Phrase() {
	delete m_words;
}

