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

  for (auto it = _cycles.begin(); it != _cycles.end(); )
  {
    const Cycle& c = *it;

    // check if another cycle is a subset, or superset, of the candidate cycle
    size_t vertsToCheck = min(c.verts.size(), verts.size());
    bool equal = all_of(verts.begin(), verts.begin() + vertsToCheck, [&c](const Vertex* v) { return c.containsVertex[v->id]; });

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
      it = _cycles.erase(it);
      continue;
    }

    // the candidate is larger, so it's not going to be added..
    addCycle = false;
    break;
  }

  if (addCycle)
  {
    // make sure the cycle has a consistent winding order
    vec3 a = verts[0]->pos;
    vec3 b = verts[1]->pos;
    vec3 c = verts[2]->pos;

    vec3 e0 = a - b;
    vec3 e1 = c - b;
    if (cross(e0, e1).z < 0)
    {
      // reverse order
      for (size_t i = 0, e = verts.size() - 1; i < verts.size() / 2; ++i, --e)
        swap(verts[i], verts[e]);
    }

    _cycles.push_back({verts, BitSet((u32)verts.size())});
    Cycle& cycle = _cycles.back();
    for (const Vertex* v : verts)
      cycle.containsVertex.Set(v->id);
  }

}

//----------------------------------------------------------------------------------
Vertex* Graph::FindOrCreateVertex(Terrain* terrain, const vec3& v)
{
  Tri* tri = terrain->FindTri(v);

  // check for an existing vertex that points to the given triangle
  auto it = _triToVerts.find(tri);
  if (it == _triToVerts.end())
  {
    // must create a new vertex. check if we can reuse an index
    int id = _recycledVertexIndices.empty() ? (int) _verts.size() : FrontPop(_recycledVertexIndices);
    Vertex* vtx = new Vertex { (tri->v0 + tri->v1 + tri->v2) / 3.f, tri, id };
    _verts.emplace_back(vtx);
    _triToVerts[tri] = vtx;
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
  _verts[idx] = nullptr;
  _recycledVertexIndices.push_back(idx);
}

//----------------------------------------------------------------------------------
void Graph::AddEdge(Vertex* a, Vertex* b)
{
  auto it = _vtxPairToEdge.find(make_pair(a->id, b->id));
  if (it == _vtxPairToEdge.end())
  {
    it = _vtxPairToEdge.find(make_pair(b->id, a->id));
    if (it == _vtxPairToEdge.end())
    {
      // add the edge if it doesn't exist
      int id = _recycledEdgeIndices.empty() ? (int) _edges.size() : FrontPop(_recycledEdgeIndices);
      _edges.emplace_back(new Edge{a, b, id});
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
  _recycledEdgeIndices.push_back(idx);

  // remove the edge from the vertices' edge lists
  a->edges.erase(remove_if(a->edges.begin(), a->edges.end(), [idx](int id) { return id == idx; }), a->edges.end());
  b->edges.erase(remove_if(b->edges.begin(), b->edges.end(), [idx](int id) { return id == idx; }), b->edges.end());
}

//----------------------------------------------------------------------------------
void Graph::Dfs()
{
  // init verts to white
  for (Vertex* v : _verts)
  {
    v->color = Color::White;
    v->parent = nullptr;
  }

  for (Vertex* v : _verts)
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
void Graph::CalcCycles(vector<Cycle>* cycles)
{
  // depth first search to detect cycles
  u32 numVerts = (u32)_verts.size();
  for (u32 i = 0; i < numVerts; ++i)
  {
    DEBUG_PRINT(printf("s: %d\n", i));
    // reset all the verts
    for (Vertex* v : _verts)
    {
      v->color = Color::White;
      v->parent = nullptr;
    }

    Vertex* v = _verts[i];
    DfsVisit(v);
  }

  for (const Cycle& c : _cycles)
  {
    printf("\ncycle: ");
    for (const Vertex* v : c.verts)
    {
      printf("%d ", v->id);
    }
  }

  *cycles = _cycles;
}


//----------------------------------------------------------------------------------
void Graph::Dump()
{
  printf("num_verts: %d\n", (int) _verts.size());
  for (const Vertex* v : _verts)
  {
    printf("v: %d\n  ", v->id);
    for (Vertex* u : v->adj)
      printf("a: %d ", u->id);
//    for (int edge : v->edges)
//      printf("e: %d ", edge);
    printf("\n");
  }

  printf("\nnum_edges: %d\n", (int) _edges.size());
  for (const Edge* e : _edges)
  {
    printf("e: %d, a: %d, d: %d\n", e->id, e->a->id, e->b->id);
  }
}

//----------------------------------------------------------------------------------
void Graph::Reset()
{
  _triToVerts.clear();
  _vtxPairToEdge.clear();

  _recycledVertexIndices.clear();
  _recycledEdgeIndices.clear();
  _cycles.clear();

  ContainerDelete(&_verts);
  ContainerDelete(&_edges);
}
