#include "utils.hpp"

#pragma once

namespace citygen
{

  struct Tri;
  struct Terrain;

  enum class Color {
    White,
    Gray,
    Black,
  };

  struct Vertex
  {
    vec3 pos;
    Tri* tri;
    int id;
    vector<int> edges;
    vector<Vertex*> adj;
    Vertex* parent;
    Color color;
  };

  struct Edge
  {
    Vertex* a;
    Vertex* b;
    int id;
  };

  struct Cycle
  {
    vector<const Vertex*> verts;
    BitSet containsVertex;
  };

  struct Graph
  {
    Vertex* FindOrCreateVertex(Terrain* terrain, const vec3& v);
    void DeleteVertex(Vertex* vtx);

    void AddEdge(Vertex* a, Vertex* b);
    void DeleteEdge(Edge* edge);

    void CalcCycles(vector<Cycle>* cycles);

    void Dfs();
    void DfsVisit(Vertex* v);

    void Dump();
    void Reset();

    void CreateCycle(const Vertex* v);

    unordered_map<Tri*, Vertex*> _triToVerts;
    // TODO: using a map here because unordered_map doen't have hash-combine over pairs..
    map<pair<int, int>, Edge*> _vtxPairToEdge;
    vector<Vertex*> _verts;
    vector<Edge*> _edges;

    // We never want to shrink the vertex array (because the indices point into it), so when vertices
    // are deleted, their slot in @verts is nulled, and their index added to the recycled list.
    deque<int> _recycledVertexIndices;
    deque<int> _recycledEdgeIndices;

    vector<Cycle> _cycles;
  };
}

