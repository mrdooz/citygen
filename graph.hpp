#pragma once

namespace citygen
{

  struct Tri;
  struct Terrain;

  struct Vertex
  {
    Tri* tri;
    int id;
    vector<Vertex*> edges;
  };

  struct Graph
  {
    Vertex* FindOrCreateVertex(Terrain* terrain, const vec3& v);
    void DeleteVertex(Vertex* vtx);

    void AddEdge(Vertex* a, Vertex* b);

    unordered_map<Tri*, Vertex*> triToVerts;
    vector<Vertex*> verts;

    // We never want to shrink the vertex array (because the indices point into it), so when vertices
    // are deleted, their slot in @verts is nulled, and their index added to the recycled list.
    deque<int> recycledIndices;
  };
}

