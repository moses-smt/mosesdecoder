// -*- mode: c++; cc-style: gnu -*-
#include <string>

namespace Moses {

  struct NBestOptions
  {
    size_t nbest_size;
    size_t factor;
    bool enabled;
    bool print_trees;
    bool only_distinct;

    bool include_alignment_info;
    bool include_segmentation;
    bool include_feature_labels;
    bool include_passthrough;

    bool include_all_factors;

    std::string output_file_path;

    bool init(Parameter const& param);

  };

}
