// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#include <string>
#include "OptionsBaseClass.h"
namespace Moses2
{

struct NBestOptions : public OptionsBaseClass {
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

  bool update(std::map<std::string,xmlrpc_c::value>const& param);

  NBestOptions();
};

}
