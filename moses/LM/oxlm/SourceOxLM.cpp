#include "moses/LM/oxlm/SourceOxLM.h"

using namespace std;
using namespace oxlm;

namespace Moses {

SourceOxLM::SourceOxLM(const string &line) : BilingualLM(line) {
}

SourceOxLM::~SourceOxLM() {}

float SourceOxLM::Score(
    vector<int>& source_words,
    vector<int>& target_words) const {
}

int SourceOxLM::LookUpNeuralLMWord(const string& str) const {
}

void SourceOxLM::initSharedPointer() const {
}

void SourceOxLM::loadModel() {
}

bool SourceOxLM::parseAdditionalSettings(
    const string& key,
    const string& value) {
}

} // namespace Moses



