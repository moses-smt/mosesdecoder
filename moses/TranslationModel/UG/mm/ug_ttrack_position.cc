#include "ug_ttrack_position.h"
namespace ugdiss
{
  namespace ttrack
  {
    Position::Position() : sid(0), offset(0) {};
    Position::Position(id_type _sid, ushort _off) : sid(_sid), offset(_off) {};
  }
}
