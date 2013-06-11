#include <stdexcept>
#include "InputFeature.h"
#include "moses/Util.h"
#include "util/check.hh"

using namespace std;

namespace Moses
{
InputFeature::InputFeature(const std::string &line)
  :StatelessFeatureFunction("InputFeature", line)
{
  for (size_t i = 0; i < m_args.size(); ++i) {
    const vector<string> &args = m_args[i];
    CHECK(args.size() == 2);

    if (args[0] == "num-input-features") {
      m_numInputScores = Scan<size_t>(args[1]);
    } else if (args[0] == "real-word-count") {
      m_numRealWordCount = Scan<size_t>(args[1]);
    } else {
      throw "Unknown argument " + args[0];
    }
  }

}

} // namespace

