// $Id$
#ifndef CREATETARGETPHRASECOLLECTION_H_
#define CREATETARGETPHRASECOLLECTION_H_
#include <list>
#include "Dictionary.h"
#include "TargetPhrase.h"

class InputType;
class WordsRange;
typedef std::list<TargetPhrase> TargetPhraseCollection;

TargetPhraseCollection const* 
CreateTargetPhraseCollection(Dictionary const*,
														 InputType const*,
														 const WordsRange&);
#endif
