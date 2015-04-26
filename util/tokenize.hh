#ifndef TOKENIZE_H
#define TOKENIZE_H

#include <string>
#include <vector>

namespace util
{

/** Split input text into a series of tokens.
 *
 * Splits on spaces and tabs, no other whitespace characters, and is not
 * locale-sensitive.
 *
 * The spaces themselves are not included.  A sequence of consecutive space/tab
 * characters count as one.
 */
inline std::vector<std::string> tokenize(const char input[])
{
  std::vector<std::string> token;
  bool betweenWords = true;
  int start = 0;
  int i;
  for(i = 0; input[i] != '\0'; i++) {
    const bool isSpace = (input[i] == ' ' || input[i] == '\t');

    if (!isSpace && betweenWords) {
      start = i;
      betweenWords = false;
    } else if (isSpace && !betweenWords) {
      token.push_back( std::string( input+start, i-start ) );
      betweenWords = true;
    }
  }
  if (!betweenWords)
    token.push_back( std::string( input+start, i-start ) );
  return token;
}

/** Split input string into a series of tokens.
 *
 * Like tokenize(const char[]), but takes a std::string.
 */
inline std::vector<std::string> tokenize(const std::string &input)
{
  return tokenize(input.c_str());
}

} // namespace util

#endif
