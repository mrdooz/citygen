#include "terrain.hpp"

using namespace citygen;

//----------------------------------------------------------------------------------
void Terrain::CreateMesh()
{
  int w = _w;
  int h = _h;
  float scale = _scale;
  float heightScale = _heightScale;

  _indices.resize((w-1) * (h-1) * 2 * 3);
  _tris.resize((w-1) * (h-1) * 2);
  _verts.resize(w*h);
  vec3* vertPtr = _verts.data();
  u32* indPtr = _indices.data();
  u8* ptr = _data;

  _minValues.x = -(w-1)/2.f * scale;
  _minValues.z = -(h-1)/2.f * scale;
  _maxValues.x = +(w-1)/2.f * scale;
  _maxValues.z = +(h-1)/2.f * scale;

  // remeber, because negative z goes into the screen, the first triangle is the furtherst left corner
  float z = _minValues.z;
  for (int i = 0; i < h; ++i)
  {
    float x = _minValues.z;
    for (int j = 0; j < w; ++j)
    {
      float y = (ptr[0] + ptr[1] + ptr[2]) / 3.f * heightScale;

      *vertPtr++ = vec3(x, y, z);

      if (i < h-1 && j < w-1)
      {
        // note where the +1 offsets in y are due to the z-order
        // v1-v3
        // v0-v2
        u32 v0 = (i+1)*w+(j+0);
        u32 v1 = (i+0)*w+(j+0);
        u32 v2 = (i+1)*w+(j+1);
        u32 v3 = (i+0)*w+(j+1);

        indPtr[0] = v0;
        indPtr[1] = v1;
        indPtr[2] = v2;

        indPtr[3] = v2;
        indPtr[4] = v1;
        indPtr[5] = v3;

        indPtr += 6;
      }

      ptr += 4;
      x += scale;
    }
    z += scale;
  }

  // calc tris
  Tri* triPtr = _tris.data();
  for (int i = 0; i < h-1; ++i)
  {
    for (int j = 0; j < w - 1; ++j)
    {
      // v1-v3
      // v0-v2
      u32 i0 = (i+1)*w+(j+0);
      u32 i1 = (i+0)*w+(j+0);
      u32 i2 = (i+1)*w+(j+1);
      u32 i3 = (i+0)*w+(j+1);

      const vec3& v0 = _verts[i0];
      const vec3& v1 = _verts[i1];
      const vec3& v2 = _verts[i2];
      const vec3& v3 = _verts[i3];

      triPtr->v0 = v0;
      triPtr->v1 = v1;
      triPtr->v2 = v2;
      triPtr->n = glm::normalize(glm::cross(v1-v0, v2-v0));
      triPtr++;

      triPtr->v0 = v2;
      triPtr->v1 = v1;
      triPtr->v2 = v3;
      triPtr->n = glm::normalize(glm::cross(v2-v3, v1-v3));
      triPtr++;
    }
  }
}

//----------------------------------------------------------------------------------
void Terrain::CalcIntersection(const vec3& org, const vec3& dir)
{
  _intersected.clear();
  Tri closestTri;
  vec3 closestPos;
  float closest = FLT_MAX;
  for (size_t i = 0; i < _tris.size(); ++i)
  {
    const Tri& tri = _tris[i];
    // pos = [t, u, v]
    vec3 pos;
    if (glm::intersectLineTriangle(org, dir, tri.v0, tri.v1, tri.v2, pos) && pos.x < closest)
    {
      closest = pos.x;
      closestTri = tri;
      closestPos = pos;
    }
  }

  if (closest != FLT_MAX)
  {
    _intersected.push_back(closestTri.v0);
    _intersected.push_back(closestTri.v1);
    _intersected.push_back(closestTri.v2);
    float u = closestPos.y;
    float v = closestPos.z;
    _intersection = (1 - u - v) * closestTri.v0 + u * closestTri.v1 + v * closestTri.v2;
  }
}

//----------------------------------------------------------------------------------
Tri* Terrain::FindTri(const vec3& pt, vec3* out)
{
  // Note, we project to x/z plane, so just ignore the y-component
  if (pt.x < _minValues.x || pt.x > _maxValues.x || pt.z < _minValues.z || pt.z > _maxValues.z)
    return nullptr;

  vec3 ofs = pt - _minValues;

  // find the quad the point is in
  int x = (int)(ofs.x / _scale);
  int z = (int)(ofs.z / _scale);

  // find the barycentric coords
  //  v1 v3
  //  v0 v2
  // P = w*v0 + v*v1 + u*v2
  float u = (ofs.x - x * _scale) / _scale;
  float v = 1 - (ofs.z - z * _scale) / _scale;
  float w = (1 - u - v);

  // if w is within [0, 1] then we are inside the bottom left triangle
  if ( w >= 0 && w <= 1)
  {
//    printf("u, %f, v: %f, w: %f\n", u, v, w);
    Tri* t = &_tris[2 * (z * (_w-1) + x) + 0];
    if (out)
      *out = w * t->v0 + v * t->v1 + u * t->v2;
    return t;
  }
  else
  {
    // upper triangle.
    Tri* t = &_tris[2 * (z * (_w-1) + x) + 1];
    if (out)
    {
      // negative u and v, so top right betcomes w
      u = 1 - u;
      v = 1 - v;
      w = (1 - u - v);
      *out = w * t->v2 + v * t->v0 + u * t->v1;
    }
    return t;
  }
}
