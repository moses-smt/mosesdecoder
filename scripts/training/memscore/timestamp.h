// memscore - in-memory phrase scoring for Statistical Machine Translation
// Christian Hardmeier, FBK-irst, Trento, 2010
// $Id$

#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <sys/time.h>

class Timestamp
{
private:
  struct timeval tv_;

public:
  typedef double time_difference;

  Timestamp() {
    gettimeofday(&tv_, NULL);
  }

  time_difference elapsed_time() const {
    struct timeval tv2;
    gettimeofday(&tv2, NULL);
    return (tv2.tv_sec - tv_.tv_sec) * 1e6 + (tv2.tv_usec - tv_.tv_usec);
  }
};

#endif
