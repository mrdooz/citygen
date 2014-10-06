#pragma once

namespace citygen
{
  struct Tri
  {
    vec3 v0, v1, v2;
    vec3 n;
  };

  struct Terrain
  {
    void CreateMesh();
    void CalcIntersection(const vec3& org, const vec3& dir);
    Tri* FindTri(const vec3& pt, vec3* out = nullptr);

    vector<vec3> _verts;
    vector<Tri> _tris;
    vector<u32> _indices;
    vector<vec3> _intersected;
    vec3 _minValues, _maxValues;
    vec3 _intersection;
    u8* _data = nullptr;
    int _w, _h, _depth;
    float _scale = 20.f;
    float _heightScale = 1.5f;
  };
}