#include "line_splitter.hh"

line_text splitLine(StringPiece textin)
{
  const char delim[] = " ||| ";
  line_text output;

  //Tokenize
  util::TokenIter<util::MultiCharacter> it(textin, util::MultiCharacter(delim));
  //Get source phrase
  output.source_phrase = *it;
  it++;
  //Get target_phrase
  output.target_phrase = *it;
  it++;
  //Get probabilities
  output.prob = *it;
  it++;
  //Get WordAllignment 1
  output.word_all1 = *it;
  it++;
  //Get WordAllignment 2
  output.word_all2 = *it;

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

