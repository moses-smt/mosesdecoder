#include "line_splitter.hh"

line_text splitLine(StringPiece textin)
{
  const char delim[] = " ||| ";
  line_text output;

  //Tokenize
  util::TokenIter<util::MultiCharacter> it(textin, util::MultiCharacter(delim));
  //Get source phrase
  output.source_phrase = *it;

  //Get target_phrase
  it++;
  output.target_phrase = *it;

  //Get probabilities
  it++;
  output.prob = *it;

  //Get WordAllignment
  it++;
  if (it == util::TokenIter<util::MultiCharacter>::end()) return output;
  output.word_align = *it;

  //Get count
  it++;
  if (it == util::TokenIter<util::MultiCharacter>::end()) return output;
  output.counts = *it;

  //Get sparse_score
  it++;
  if (it == util::TokenIter<util::MultiCharacter>::end()) return output;
  output.sparse_score = *it;

  //Get property
  it++;
  if (it == util::TokenIter<util::MultiCharacter>::end()) return output;
  output.property = *it;

  return output;
}

std::vector<unsigned char> splitWordAll1(StringPiece textin)
{
  const char delim[] = " ";
  const char delim2[] = "-";
  std::vector<unsigned char> output;

  //Split on space
  util::TokenIter<util::MultiCharacter> it(textin, util::MultiCharacter(delim));

  //For each int
  while (it) {
    //Split on dash (-)
    util::TokenIter<util::MultiCharacter> itInner(*it, util::MultiCharacter(delim2));

    //Insert the two entries in the vector. User will read entry 0 and 1 to get the first,
    //2 and 3 for second etc. Use unsigned char instead of int to save space, as
    //word allignments are all very small numbers that fit in a single byte
    output.push_back((unsigned char)(atoi(itInner->data())));
    itInner++;
    output.push_back((unsigned char)(atoi(itInner->data())));
    it++;
  }

  return output;

}

