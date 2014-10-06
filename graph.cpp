#include "graph.hpp"
#include "terrain.hpp"

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

  verts.emplace_back(new Vertex { tri, id });
  return verts.back();
}

//----------------------------------------------------------------------------------
void Graph::DeleteVertex(Vertex* vtx)
{
  int idx = vtx->id;
  delete vtx;
  verts[idx] = nullptr;
  recycledIndices.push_back(idx);
}

//----------------------------------------------------------------------------------
void Graph::AddEdge(Vertex* a, Vertex* b)
{

}
