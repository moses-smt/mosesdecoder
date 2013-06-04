
#include <cassert>
#include <vector>
#include "Alignments.h"
#include "moses/Util.h"

using namespace std;
using namespace Moses;

Alignments::Alignments(const std::string &str, size_t sourceSize, size_t targetSize)
  :m_alignS2T(sourceSize)
  ,m_alignT2S(targetSize)
{
  vector<string> toks   = Tokenize(str, " ");
  for (size_t i = 0; i < toks.size(); ++i) {
    string &tok = toks[i];

    vector<int> point = Tokenize<int>(tok, "-");
    assert(point.size() == 2);

    std::map<int, int>::iterator iter;

    // m_alignedS2T
    std::map<int, int> &targets = m_alignS2T[ point[0] ];
    iter = targets.find(point[1]);
    if (iter == targets .end()) {
      targets[ point[1] ] = 0;
    } else {
      ++(iter->second);
    }

    // m_alignedToS
    std::map<int, int> &sources = m_alignT2S[ point[1] ];
    iter = sources.find(point[0]);
    if (iter == targets .end()) {
      sources[ point[0] ] = 0;
    } else {
      ++(iter->second);
    }
  }

}


