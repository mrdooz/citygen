#include "graph.hpp"
#include "terrain.hpp"
#include "utils.hpp"

using namespace citygen;

//----------------------------------------------------------------------------------
Vertex* Graph::FindOrCreateVertex(Terrain* terrain, const vec3& v)
{
  Tri* tri = terrain->FindTri(v);

  // check for an existing vertex that points to the given triangle
  auto it = triToVerts.find(tri);
  if (it != triToVerts.end())
  {
    return it->second;
  }

  // must create a new vertex. check if we can reuse an index
  int id;
  if (!recycledIndices.empty())
  {
    id = recycledIndices.front();
    recycledIndices.pop_front();
  }
  else
  {
    id = (int)verts.size();
  }

  verts.emplace_back(new Vertex { (tri->v0 + tri->v1 + tri->v2) / 3.f, tri, id });
  return verts.back();
}

//----------------------------------------------------------------------------------
void Graph::DeleteVertex(Vertex* vtx)
{
  // delete the vertex, null the entry in the vertex vector, and add the vertice's
  // index to the list of recycled indices
  int idx = vtx->id;
  delete vtx;
  verts[idx] = nullptr;
  recycledIndices.push_back(idx);
}

//----------------------------------------------------------------------------------
void Graph::AddEdge(Vertex* a, Vertex* b)
{
  a->edges.push_back(b);
  b->edges.push_back(a);
}


//----------------------------------------------------------------------------------
void Graph::CalcCycles()
{
  if (verts.size() < 3)
    return;

  BitSet visited((u32)verts.size());

  while (true)
  {
    // start with the left-most unvisited vertex
    Vertex* start = nullptr;
    for (size_t i = 0, e = verts.size(); i < e; ++i)
    {
      Vertex* cand = verts[0];
      if (visited[cand->id])
        continue;

      if (start == nullptr || cand->pos.x < start->pos.x)
      {
        start = cand;
      }
    }

    // if no candidates are found, break
    if (!start)
      break;

    // start a DFS, traversing all unvisited adjacent verts.
    BitSet cycleVisited((u32)verts.size());
    cycleVisited.Set(start->id);

    // TODO: i've seen some DFS code that differentiates between not visited, in progress and done.. is this
    // useful?
    deque<const Vertex*> frontier;
    for (const Vertex* x : start->edges)
    {
      cycleVisited.Set(x->id);
      frontier.push_back(x);
    }

    while (!frontier.empty())
    {
      const Vertex* a = frontier.front();
      frontier.pop_front();

      // if a has an edge to the starting vertex, then we've found a cycle
      for (const Vertex* x : a->edges)
      {
        if (x == start)
        {
          // found cycle
          int a = 10;
        }
      }
    }

  }

}
