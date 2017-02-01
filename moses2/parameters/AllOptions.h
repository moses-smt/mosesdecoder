// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#include <string>
#include <boost/shared_ptr.hpp>
#include "OptionsBaseClass.h"
#include "SearchOptions.h"
#include "CubePruningOptions.h"
#include "NBestOptions.h"
#include "ReorderingOptions.h"
#include "ContextParameters.h"
#include "InputOptions.h"
#include "MBR_Options.h"
#include "LMBR_Options.h"
#include "ReportingOptions.h"
#include "OOVHandlingOptions.h"
#include "ServerOptions.h"
#include "SyntaxOptions.h"

namespace Moses2
{
struct
    AllOptions : public OptionsBaseClass {
  typedef boost::shared_ptr<AllOptions const> ptr;
  SearchOptions         search;
  CubePruningOptions      cube;
  NBestOptions           nbest;
  ReorderingOptions reordering;
  ContextParameters    context;
  InputOptions           input;
  MBR_Options              mbr;
  LMBR_Options            lmbr;
  ReportingOptions      output;
  OOVHandlingOptions       unk;
  ServerOptions       server;
  SyntaxOptions         syntax;
  bool mira;
  bool use_legacy_pt;
  // StackOptions      stack;
  // BeamSearchOptions  beam;
  bool init(Parameter const& param);
  bool sanity_check();
  AllOptions();
  AllOptions(Parameter const& param);

  bool update(std::map<std::string,xmlrpc_c::value>const& param);
  bool NBestDistinct() const;

};

}
