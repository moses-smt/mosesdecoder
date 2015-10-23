/*
 * Phrase.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <cstddef>

class Word;

class Phrase {
public:
	Phrase(size_t size);
	virtual ~Phrase();

protected:
  size_t m_size;
  Word *m_words;


};

