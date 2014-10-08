#include "graph.hpp"
#include "terrain.hpp"
#include "utils.hpp"

using namespace citygen;

#define DEBUG_OUTPUT 0

#if DEBUG_OUTPUT
#define DEBUG_PRINT(x) x
#else
#define DEBUG_PRINT(x)
#endif

//----------------------------------------------------------------------------------
void Graph::CreateCycle(const Vertex* v)
{
  // Collect all the verts
  vector<const Vertex*> verts;
  do
  {
    verts.push_back(v);
  } while ((v = v->parent));

  // check if any other cycle contains these verts. if this cycle is smaller,
  // throw out the larger cycle

  bool addCycle = true;

  for (auto it = cycles.begin(); it != cycles.end(); )
  {
    const Cycle& c = *it;
    bool equal = true;

    // check if another cycle is a subset, or superset, of the candidate cycle
    size_t vertsToCheck = min(c.verts.size(), verts.size());
    for (size_t i = 0; i < vertsToCheck; ++i)
    {
      const Vertex* v = verts[i];
      if (!c.containsVertex[v->id])
      {
        equal = false;
        break;
      }
    }

    if (!equal)
    {
      // the cycles don't overlap, so continue testing the next one..
      ++it;
      continue;
    }

    // the cycles are equal, so check which one to discard
    if (c.verts.size() > verts.size())
    {
      // discard an existing cycle, and (perhaps) replace with the smaller one
      it = cycles.erase(it);
      continue;
    }

    // the candidate is larger, so it's not going to be added..
    addCycle = false;
    break;
  }

  if (addCycle)
  {
    cycles.push_back({verts, BitSet(verts.size())});
    Cycle& c = cycles.back();
    for (const Vertex* v : verts)
      c.containsVertex.Set(v->id);
  }
}

//----------------------------------------------------------------------------------
Vertex* Graph::FindOrCreateVertex(Terrain* terrain, const vec3& v)
{
  Tri* tri = terrain->FindTri(v);

  // check for an existing vertex that points to the given triangle
  auto it = triToVerts.find(tri);
  if (it == triToVerts.end())
  {
    // must create a new vertex. check if we can reuse an index
    int id = recycledVertexIndices.empty() ? (int)verts.size() : FrontPop(recycledVertexIndices);
    Vertex* vtx = new Vertex { (tri->v0 + tri->v1 + tri->v2) / 3.f, tri, id };
    verts.emplace_back(vtx);
    triToVerts[tri] = vtx;
    return vtx;
  }

  return it->second;
}

//----------------------------------------------------------------------------------
void Graph::DeleteVertex(Vertex* vtx)
{
  // delete the vertex, null the entry in the vertex vector, and add the vertice's
  // index to the list of recycled indices
  int idx = vtx->id;
  delete vtx;
  verts[idx] = nullptr;
  recycledVertexIndices.push_back(idx);
}

//----------------------------------------------------------------------------------
void Graph::AddEdge(Vertex* a, Vertex* b)
{
  auto it = vtxPairToEdge.find(make_pair(a->id, b->id));
  if (it == vtxPairToEdge.end())
  {
    it = vtxPairToEdge.find(make_pair(b->id, a->id));
    if (it == vtxPairToEdge.end())
    {
      // add the edge if it doesn't exist
      int id = recycledEdgeIndices.empty() ? (int)edges.size() : FrontPop(recycledEdgeIndices);
      edges.emplace_back(new Edge{a, b, id});
      a->edges.push_back(id);
      b->edges.push_back(id);
      a->adj.push_back(b);
//      b->adj.push_back(a);
    }
  }
}

//----------------------------------------------------------------------------------
void Graph::DeleteEdge(Edge* edge)
{
  int idx = edge->id;
  Vertex* a = edge->a;
  Vertex* b = edge->b;

  delete edge;
  recycledEdgeIndices.push_back(idx);

  // remove the edge from the vertices' edge lists
  a->edges.erase(remove_if(a->edges.begin(), a->edges.end(), [idx](int id) { return id == idx; }), a->edges.end());
  b->edges.erase(remove_if(b->edges.begin(), b->edges.end(), [idx](int id) { return id == idx; }), b->edges.end());
}

//----------------------------------------------------------------------------------
void Graph::DfsCycles()
{
  u32 numVerts = (u32)verts.size();
  for (u32 i = 0; i < numVerts; ++i)
  {
    DEBUG_PRINT(printf("s: %d\n", i));
    // reset all the verts
    for (Vertex* v : verts)
    {
      v->color = Color::White;
      v->parent = nullptr;
    }

    Vertex* v = verts[i];
    DfsVisit(v);
  }

  for (const Cycle& c : cycles)
  {
    printf("\ncycle: ");
    for (const Vertex* v : c.verts)
    {
      printf("%d ", v->id);
    }
  }

}

//----------------------------------------------------------------------------------
void Graph::Dfs()
{
  // init verts to white
  for (Vertex* v : verts)
  {
    v->color = Color::White;
    v->parent = nullptr;
  }

  for (Vertex* v : verts)
  {
    DEBUG_PRINT(printf("v: %d\n", v->id));
    if (v->color == Color::White)
      DfsVisit(v);
  }
}

//----------------------------------------------------------------------------------
void Graph::DfsVisit(Vertex* v)
{
  DEBUG_PRINT(printf("e: %d ", v->id));
  v->color = Color::Gray;

  for (Vertex* u : v->adj)
  {
    DEBUG_PRINT(printf("a: %d ", u->id));
    switch (u->color)
    {
      case Color::White:
      {
        u->parent = v;
        DfsVisit(u);
        break;
      }

      case Color::Gray:
      {
        CreateCycle(v);
        break;
      }

      default: break;
    }
  }

  v->color = Color::Black;
}

//----------------------------------------------------------------------------------
void Graph::CalcCycles()
{
  DfsCycles();
}


//----------------------------------------------------------------------------------
void Graph::Dump()
{
  printf("num_verts: %d\n", (int)verts.size());
  for (const Vertex* v : verts)
  {
    printf("v: %d\n  ", v->id);
    for (Vertex* u : v->adj)
      printf("a: %d ", u->id);
//    for (int edge : v->edges)
//      printf("e: %d ", edge);
    printf("\n");
  }

  printf("\nnum_edges: %d\n", (int)edges.size());
  for (const Edge* e : edges)
  {
    printf("e: %d, a: %d, d: %d\n", e->id, e->a->id, e->b->id);
  }
}

//----------------------------------------------------------------------------------
void Graph::Reset()
{
  triToVerts.clear();
  vtxPairToEdge.clear();

  for (Vertex* v : verts)
    delete v;
  verts.clear();

  for (Edge* e : edges)
    delete e;
  edges.clear();

  recycledVertexIndices.clear();
  recycledEdgeIndices.clear();
  cycles.clear();
}
