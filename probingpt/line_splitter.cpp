#include "line_splitter.h"

namespace probingpt
{

line_text splitLine(const StringPiece &textin, bool scfg)
{
  const char delim[] = "|||";
  line_text output;

  //Tokenize
  util::TokenIter<util::MultiCharacter> it(textin, util::MultiCharacter(delim));
  //Get source phrase
  output.source_phrase = Trim(*it);
  //std::cerr << "output.source_phrase=" << output.source_phrase << "AAAA" << std::endl;

  //Get target_phrase
  it++;
  output.target_phrase = Trim(*it);
  //std::cerr << "output.target_phrase=" << output.target_phrase << "AAAA" << std::endl;

  if (scfg) {
    /*
    std::cerr << "output.source_phrase=" << output.source_phrase << std::endl;
    std::cerr << "output.target_phrase=" << output.target_phrase << std::endl;
    reformatSCFG(output);
    std::cerr << "output.source_phrase=" << output.source_phrase << std::endl;
    std::cerr << "output.target_phrase=" << output.target_phrase << std::endl;
    */
  }

  //Get probabilities
  it++;
  output.prob = Trim(*it);
  //std::cerr << "output.prob=" << output.prob << "AAAA" << std::endl;

  //Get WordAllignment
  it++;
  if (it == util::TokenIter<util::MultiCharacter>::end()) return output;
  output.word_align = Trim(*it);
  //std::cerr << "output.word_align=" << output.word_align << "AAAA" << std::endl;

  //Get count
  it++;
  if (it == util::TokenIter<util::MultiCharacter>::end()) return output;
  output.counts = Trim(*it);
  //std::cerr << "output.counts=" << output.counts << "AAAA" << std::endl;

  //Get sparse_score
  it++;
  if (it == util::TokenIter<util::MultiCharacter>::end()) return output;
  output.sparse_score = Trim(*it);
  //std::cerr << "output.sparse_score=" << output.sparse_score << "AAAA" << std::endl;

  //Get property
  it++;
  if (it == util::TokenIter<util::MultiCharacter>::end()) return output;
  output.property = Trim(*it);
  //std::cerr << "output.property=" << output.property << "AAAA" << std::endl;

  return output;
}

std::vector<unsigned char> splitWordAll1(const StringPiece &textin)
{
  const char delim[] = " ";
  const char delim2[] = "-";
  std::vector<unsigned char> output;

  //Case with no word alignments.
  if (textin.size() == 0) {
    return output;
  }

  //Split on space
  util::TokenIter<util::MultiCharacter> it(textin, util::MultiCharacter(delim));

  //For each int
  while (it) {
    //Split on dash (-)
    util::TokenIter<util::MultiCharacter> itInner(*it,
        util::MultiCharacter(delim2));

    //Insert the two entries in the vector. User will read entry 0 and 1 to get the first,
    //2 and 3 for second etc. Use unsigned char instead of int to save space, as
    //word allignments are all very small numbers that fit in a single byte
    output.push_back((unsigned char) (atoi(itInner->data())));
    itInner++;
    output.push_back((unsigned char) (atoi(itInner->data())));
    it++;
  }

  return output;

}

void reformatSCFG(line_text &output)
{

}

}

