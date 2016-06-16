#include "automaton.hh"

namespace {
void CheckEqual(const lm::FullScoreReturn& lhs, const lm::FullScoreReturn& rhs) {
#ifdef NDEBUG
#define AUTOMATON_NDEBUG_WAS_SET
#undef NDEBUG
#endif
    assert(lhs.prob == rhs.prob);
    assert(lhs.independent_left == rhs.independent_left);
    assert(lhs.ngram_length == rhs.ngram_length);
    assert(lhs.rest == rhs.rest);
#ifdef AUTOMATON_NDEBUG_WAS_SET
#define NDEBUG
#undef AUTOMATON_NDEBUG_WAS_SET
#endif
}
}
int main(int argc, char* argv[]){
    if (argc < 4) {
        std::cerr << "Usage: <pipeline size> <arpa file> <test file>" << std::endl;
        return 1;
    }
    int pipeline_size = std::stoi(std::string(argv[1]));
    std::string arpa(argv[2]);
    std::string test(argv[3]);

    lm::ngram::Config config;
    config.arpa_complain = lm::ngram::Config::NONE;
    config.messages = nullptr;
    config.positive_log_probability = lm::SILENT;
    config.probing_multiplier = 2.0;
    lm::ngram::ProbingModel model(arpa.data(), config);
    lm::Pipeline pipeline(pipeline_size, {model.GetSearch(), model.Order()});
    
    util::FilePiece in(test.data());
    StringPiece word;
    auto model_score = 0.0;
    auto pipe_score = 0.0;

    while (true) {
        lm::ngram::State in_state, out_state;
        if (in.ReadWordSameLine(word)) {
            lm::WordIndex vocab = model.GetVocabulary().Index(word);
            auto ret = model.FullScore(model.BeginSentenceState(), vocab, out_state); 
            model_score += ret.prob;
            auto callback = [=, &pipe_score](const lm::FullScoreReturn& r){CheckEqual(ret, r); pipe_score += r.prob;};
            pipeline.FullScore(model.BeginSentenceState(), vocab, callback);
            in_state = out_state;
        }

        while(in.ReadWordSameLine(word)) {
            lm::WordIndex vocab = model.GetVocabulary().Index(word);
            auto ret = model.FullScore(in_state, vocab, out_state);
            model_score += ret.prob;
            auto callback = [=, &pipe_score](const lm::FullScoreReturn& r){CheckEqual(ret, r); pipe_score += r.prob;};
            pipeline.AppendWord(vocab, callback);
            in_state = out_state;
        }

        try {
            UTIL_THROW_IF('\n' != in.get(), util::Exception, "FilePiece is confused.");
        } catch (const util::EndOfFileException &e) { break; }

        
        auto ret = model.FullScore(in_state, model.GetVocabulary().EndSentence(), out_state);
        model_score += ret.prob;
        auto callback = [=, &pipe_score](const lm::FullScoreReturn& r){CheckEqual(ret, r); pipe_score += r.prob;};
        pipeline.AppendWord(model.GetVocabulary().EndSentence(), callback);
    }
    pipeline.Drain();
    std::cout << model_score << " " << pipe_score << std::endl;
}
