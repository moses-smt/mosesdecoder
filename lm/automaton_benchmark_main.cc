#include "automaton.hh"
#include "util/usage.hh"
#include <iomanip>
#include <limits>

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

struct Config {
    int pipeline_size_start;
    int pipeline_size_end;
    char* model_file;
    std::string type;
    int fd_in;
};

template <typename Callback, typename Width>
void PipelineScore(lm::Pipeline<Callback>& pipeline, const lm::ngram::ProbingModel& model, const Config& options){
    const Width kEOS = model.GetVocabulary().EndSentence();
    const lm::ngram::State begin_state = model.BeginSentenceState();
    std::array<Width, 49806> buff;
    util::SeekOrThrow(options.fd_in, 0);

    //start timer
    auto time = util::CPUTime();
    bool new_sentence = true;
    while (true) {
      std::size_t got = util::ReadOrEOF(options.fd_in, buff.begin(), buff.size() * sizeof(Width));
      if (!got) break;
      UTIL_THROW_IF2(got % sizeof(Width), "File size not a multiple of vocab id size " << sizeof(Width));
      auto end = buff.begin() + got / sizeof(Width);
      auto curr = buff.begin();
      while(curr != end) {
          if (new_sentence) pipeline.FullScore(begin_state, *curr);
          else pipeline.AppendWord(*curr);
          new_sentence = *curr++ == kEOS;
      }
    }
    pipeline.Drain();
    //stop timer
    time = util::CPUTime() - time;
    std::cout << time << ' ';
}

template<typename Width>
void ModelScore(const lm::ngram::ProbingModel& model, const Config& options){
    const Width kEOS = model.GetVocabulary().EndSentence();
    const lm::ngram::State* const begin_state = &model.BeginSentenceState();
    const lm::ngram::State *in_state = begin_state;
    std::array<Width, 49806> buff;
    lm::ngram::State states[3];
    long double score = 0.0;

    //start timer
    auto time = util::CPUTime();
    bool new_sentence = true;
    while (true) {
      std::size_t got = util::ReadOrEOF(options.fd_in, buff.begin(), buff.size() * sizeof(Width));
      if (!got) break;
      UTIL_THROW_IF2(got % sizeof(Width), "File size not a multiple of vocab id size " << sizeof(Width));
      auto even_end = buff.begin() + ((got / sizeof(Width)) & ~1);
      auto curr = buff.begin();
      while(curr != even_end) {
        score += model.FullScore(*in_state, *curr, states[1]).prob;
        in_state = (*curr++ == kEOS) ? begin_state : &states[1];
        score += model.FullScore(*in_state, *curr, states[0]).prob;
        in_state = (*curr++ == kEOS) ? begin_state : &states[0];
      }
      if (got & 1) {
          score += model.FullScore(*in_state, *curr, states[2]).prob;
          in_state = *curr == kEOS ? begin_state : &states[2];
      }
    }
    //stop timer
    time = util::CPUTime() - time;
    std::cout << time << ' ';
    std::cerr << "Score(model) : " << std::setprecision(std::numeric_limits<long double>::digits10 + 1) << score << std::endl;
}

template<typename Width>
void DispatchFunction(lm::ngram::ProbingModel& model, const Config& options){
    if (options.type == "probing") ModelScore<Width>(model, options);
    else if (options.type == "pipeline") {
        long double score = 0.0;
        const auto callback = [&score](const lm::FullScoreReturn& r){score += r.prob;};
        typename lm::ngram::NGramAutomaton<lm::ngram::BackoffValue, decltype(callback)>::Construct construct{model.GetSearch(), callback};
        for (std::size_t pipeline_size = options.pipeline_size_start; pipeline_size <= options.pipeline_size_end; ++pipeline_size) {
            score = 0.0;
            lm::Pipeline<decltype(callback)> pipeline(pipeline_size, construct);
            PipelineScore<decltype(callback), Width>(pipeline, model, options);
            std::cerr << "Score(pipeline): " << std::setprecision(std::numeric_limits<long double>::digits10 + 1) << score << std::endl;
        }
    }
}

void DispatchWidth(lm::ngram::ProbingModel& model, const Config& options) {
  uint64_t bound = model.GetVocabulary().Bound();
  if (bound <= 256) {
    DispatchFunction<uint8_t>(model, options);
  } else if (bound <= 65536) {
    DispatchFunction<uint16_t>(model, options);
  } else if (bound <= (1ULL << 32)) {
    DispatchFunction<uint32_t>(model, options);
  } else {
    DispatchFunction<uint64_t>(model, options);
  }
}


int main(int argc, char* argv[]){
    if (argc < 6) {
        std::cerr << argv[0] <<" pipeline_size_start pipeline_size_end model_file query_file {probing|pipeline}" << std::endl;
        return 1;
    }
    int pipeline_size_start = std::stoi(std::string(argv[1]));
    int pipeline_size_end = std::stoi(std::string(argv[2]));
    char* model_file(argv[3]);
    util::scoped_fd in_fd(util::OpenReadOrThrow(argv[4]));
    std::string type(argv[5]);
    Config options{pipeline_size_start, pipeline_size_end, model_file, type, in_fd.get()};

    lm::ngram::Config config;
    config.arpa_complain = lm::ngram::Config::ALL;
    config.messages = &std::cout;
    config.positive_log_probability = lm::SILENT;
    config.probing_multiplier = 1.5;
    lm::ngram::ProbingModel model(options.model_file, config);

    DispatchWidth(model, options);
}
