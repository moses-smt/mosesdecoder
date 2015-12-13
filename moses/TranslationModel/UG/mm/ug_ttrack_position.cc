// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#include "ug_ttrack_position.h"
namespace sapt
{
  namespace ttrack
  {
    using tpt::id_type;
    Position::Position() : sid(0), offset(0) {};
    Position::Position(id_type _sid, ushort _off) : sid(_sid), offset(_off) {};

  }
}
