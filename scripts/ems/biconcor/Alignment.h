#include "Vocabulary.h"

#pragma once

#define LINE_MAX_LENGTH 10000

class Alignment 
{
public:
	typedef unsigned int INDEX;
	
private:
	char *m_array;
	INDEX *m_sentenceEnd;
	INDEX m_size;
	INDEX m_sentenceCount;
	char m_unaligned[ 256 ];

public:
	~Alignment();

	void Create( string fileName );
	bool PhraseAlignment( INDEX sentence, char target_length,
	                                 char source_start, char source_end,
	                                 char &target_start, char &target_end,
																	 char &pre_null, char &post_null );
	void Load( string fileName );
	void Save( string fileName );
	vector<string> Tokenize( const char input[] );
};
