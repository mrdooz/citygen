#include "graph.hpp"
#include "terrain.hpp"
#include "utils.hpp"

using namespace citygen;

//----------------------------------------------------------------------------------
namespace
{
  float MinX(const Edge* e)
  {
    return min(e->a->pos.x, e->b->pos.x);
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
  u32 numVerts = verts.size();
  for (u32 i = 0; i < numVerts; ++i)
  {
    printf("s: %d\n", i);
    // reset all the verts
    for (Vertex* v : verts)
    {
      v->color = Color::White;
      v->parent = nullptr;
    }

    Vertex* v = verts[i];
    DfsVisit(v);
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
    if (v->color == Color::White)
      DfsVisit(v);
  }

}

//----------------------------------------------------------------------------------
void Graph::DfsVisit(Vertex* v)
{
  v->color = Color::Gray;

  for (Vertex* u : v->adj)
  {
    if (u->color == Color::White)
    {
      u->parent = v;
      DfsVisit(u);
    }
    else
    {
      // back edge found => cycle detected
      printf("%d ", u->id);
      Vertex* cur = v;
      do
      {
        printf("%d ", cur->id);
        cur = cur->parent;
      } while (cur);
      printf("\n");
    }
  }

  v->color = Color::Black;
}

//----------------------------------------------------------------------------------
void Graph::CalcCycles()
{
  Dump();
  //Dfs();
  DfsCycles();
  return;

  if (verts.size() < 3)
    return;

  // Note, we keep track of visited edges, not vertices
  BitSet visited((u32)edges.size());
  vector<const Vertex*> parents(verts.size());

  Edge* start = nullptr;

  while (true)
  {
    // start with the left-most unvisited edge
    float minX = FLT_MAX;
    for (Edge* cand : edges)
    {
      if (visited[cand->id])
        continue;

      float t = MinX(cand);
      if (t < minX)
      {
        minX = t;
        start = cand;
      }
    }

    // if no candidates are found, break
    if (!start)
      break;

    const Vertex* startVtx = start->a;

    // start a DFS, traversing all unvisited edges
    BitSet cycleVisited((u32)edges.size());
    cycleVisited.Set(start->id);

    deque<const Vertex*> frontier;
    parents[startVtx->id] = nullptr;
    for (int edgeIdx : startVtx->edges)
    {
      // cheese to avoid the back edge
      if (cycleVisited.IsSet(edgeIdx))
        continue;

      const Edge* edge = edges[edgeIdx];
      // add the vertex that we didn't come from..
      const Vertex* v = edge->a == startVtx ? edge->b : edge->a;
      frontier.push_back(v);
      parents[v->id] = startVtx;
    }

    while (!frontier.empty())
    {
      const Vertex* a = FrontPop(frontier);
      printf("f: %d\n", a->id);

      // if a has an unvisited edge to the starting vertex, then we've found a cycle
      for (int edgeIdx : a->edges)
      {
        printf("e: %d (%s)\n", edgeIdx, cycleVisited[edgeIdx] ? "visited" : "not_visited");

        if (cycleVisited[edgeIdx])
          continue;

        const Edge* edge = edges[edgeIdx];
        if (edge->a == startVtx || edge->b == startVtx)
        {
          // found cycle
          // remove all edges from the starting vertex to the first junction as invalid
          const Vertex* cur = edge->a == startVtx ? edge->b : edge->a;
          printf("cycle:\n");
          do
          {
            printf("%d, ", cur->id);
            cur = parents[cur->id];
          } while (cur);

          return;
        }
        else
        {
          // no cycle, so add the next vertex to the frontier
          const Vertex* v = edge->a == a ? edge->b : edge->a;
          frontier.push_back(v);
          parents[v->id] = a;
          cycleVisited.Set(edge->id);
        }
      }
    }
  }

}


//----------------------------------------------------------------------------------
void Graph::Dump()
{
  printf("num_verts: %d\n", verts.size());
  for (const Vertex* v : verts)
  {
    printf("v: %d\n  ", v->id);
    for (Vertex* u : v->adj)
      printf("a: %d ", u->id);
//    for (int edge : v->edges)
//      printf("e: %d ", edge);
    printf("\n");
  }

  printf("\nnum_edges: %d\n", edges.size());
  for (const Edge* e : edges)
  {
    printf("e: %d, a: %d, d: %d\n", e->id, e->a->id, e->b->id);
  }
}
