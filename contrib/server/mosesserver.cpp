#include "moses/ExportInterface.h"
// The separate moses server executable has been phased out.

/** main function of the command line version of the decoder **/
int main(int argc, char const** argv)
{
  // Map double-dash long options back to single-dash long options
  // as used in legacy moses.
  for (int i = 1; i < argc; ++i)
    {
      if (argv[i][0] == '-' && argv[i][1] == '-')
	++argv[i];
    }
  char const* argv2[argc+1];
  for (int i = 0; i < argc; ++i)
    argv2[i] = argv[i];
  argv2[argc] = "--server";
  return decoder_main(argc+1, argv2);
}
