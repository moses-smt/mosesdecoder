#ifndef UTIL_UNISTD_H
#define UTIL_UNISTD_H

#if (defined(_WIN32) || defined(_WIN64)) && !defined(__MINGW32__)

// Windows doesn't define <unistd.h>
//
// So we define what we need here instead:
//
#define STDIN_FILENO=0
#define STDOUT_FILENO=1


#else // Huzzah for POSIX!

#include <unistd.h>

#endif



#endif // UTIL_UNISTD_H
