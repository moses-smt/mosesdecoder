//-*- c++ -*-
#pragma once
#include <vector>
#include <string>
namespace Moses {

  // Function to splice the argument list (e.g. before handing it over to
  // Moses LoadParam() function. /filter/ is a vector of argument names
  // and the number of arguments after each of them
  void
  filter_arguments(int const argc_in, char const* const* const argv_in,
		   int & argc_moses, char*** argv_moses,
		   int & argc_other, char*** argv_other,
		   std::vector<std::pair<std::string, int> > const& filter);


} // namespace Moses
