#include "utils.hpp"
#include "protocol/city.pb.h"

using namespace citygen;

namespace citygen
{
  //----------------------------------------------------------------------------------
  vec3 FromProtocol(const protocol::Vector3& v)
  {
    return vec3(v.x(), v.y(), v.z());
  }

  //----------------------------------------------------------------------------------
  void ToProtocol(const vec3& v, protocol::Vector3* out)
  {
    out->set_x(v.x);
    out->set_y(v.y);
    out->set_z(v.z);
  }

}
