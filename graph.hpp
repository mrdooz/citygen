#pragma once

namespace citygen
{

  struct Tri;
  struct Terrain;

  struct Vertex
  {
    vec3 pos;
    Tri* tri;
    int id;
    vector<int> edges;
  };

  struct Edge
  {
    // note, vertices are stored so a < b
    Vertex* a;
    Vertex* b;
    int id;
  };

  struct Graph
  {
    Vertex* FindOrCreateVertex(Terrain* terrain, const vec3& v);
    void DeleteVertex(Vertex* vtx);

    void AddEdge(Vertex* a, Vertex* b);
    void DeleteEdge(Edge* edge);

    void CalcCycles();

    unordered_map<Tri*, Vertex*> triToVerts;
    // TODO: using a map here because unordered_map doen't have hash-combine over pairs..
    map<pair<int, int>, Edge*> vtxPairToEdge;
    vector<Vertex*> verts;
    vector<Edge*> edges;

    // We never want to shrink the vertex array (because the indices point into it), so when vertices
    // are deleted, their slot in @verts is nulled, and their index added to the recycled list.
    deque<int> recycledVertexIndices;
    deque<int> recycledEdgeIndices;
  };
}

