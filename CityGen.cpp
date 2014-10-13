#include "citygen.hpp"
#include "imgui_helpers.hpp"
#include "utils.hpp"
#pragma warning(push)
#pragma warning(disable: 4244 4267)
#include "protocol/city.pb.h"
#pragma warning(pop)

using namespace citygen;

CityGen citygen::g_city;
Terrain citygen::g_terrain;

//----------------------------------------------------------------------------------
void CellParameterSet::ToProtocol(protocol::CellParameterSet* proto) const
{
  proto->set_cell_id(cellId);
  proto->set_segment_size(segmentSize);
  proto->set_segment_size_deviation(segmentSizeDeviation);
  proto->set_degree(degree);
  proto->set_degree_deviation(degreeDeviation);
  proto->set_snap_size(snapSize);
  proto->set_snap_size_deviation(snapSizeDeviation);
  proto->set_connectivity(connectivity);
}

//----------------------------------------------------------------------------------
void CellParameterSet::FromProtocol(const protocol::CellParameterSet& proto)
{
  cellId                = proto.cell_id();
  segmentSize           = proto.segment_size();
  segmentSizeDeviation  = proto.segment_size_deviation();
  degree                = proto.degree();
  degreeDeviation       = proto.degree_deviation();
  snapSize              = proto.snap_size();
  snapSizeDeviation     = proto.snap_size_deviation();
  connectivity          = proto.connectivity();
}

//----------------------------------------------------------------------------------
void StepSettings::ToProtocol(protocol::StepSettings* proto) const
{
  proto->set_num_segments(numSegments);
  proto->set_step_size(stepSize);
  proto->set_deviation(deviation);
  proto->set_road_height(roadHeight);
}

//----------------------------------------------------------------------------------
void StepSettings::FromProtocol(const protocol::StepSettings& proto)
{
  numSegments = proto.num_segments();
  stepSize    = proto.step_size();
  deviation   = proto.deviation();
  roadHeight  = proto.road_height();
}

//----------------------------------------------------------------------------------
CityGen::CityGen()
    : _arcball(nullptr)
    , _drawDebugLines(true)
    , _drawNormals(false)
#if _WIN32
    , _configBase("/projects/citygen/")
  #else
    , _configBase("/Users/dooz/projects/citygen/")
#endif
{

  _stateFlags.Set(StateFlagsF::Paused);
  _clickFlags.Set(ClickFlagsF::FirstClick);
}

//----------------------------------------------------------------------------------
CityGen::~CityGen()
{
  SAFE_DELETE(_arcball)
}

//----------------------------------------------------------------------------------
bool CityGen::Init()
{
//
//   _fileWatcher.AddFileWatch(GetAppRoot() + "config/editor_settings.pb", 0, true, 0, [this](const string& filename, void* token)
//   {
//     return LoadProto(filename.c_str(), &_settings);
//   });
//
//   _fileWatcher.AddFileWatch(GetAppRoot() + "config/editor_styles.pb", 0, true, 0, [this](const string& filename, void* token)
//   {
//     return _styleFactory.Init(filename.c_str());
//   });
//
//
//   protocol::effect::plexus::PlexusConfig plexusSettings;
//   if (!LoadProto("config/plexus1.pb", &plexusSettings, true))
//     return false;

  //_plexus = FromProtocol(plexusSettings);

  _configFile = _configBase + "config/city1.pb";
  LoadSettings(_configFile.c_str());

  if (!g_terrain.Init("noise.tga"))
    return false;

  PostLoadSetup();

  InitGL();
  InitImGui();

  int w, h;
  glfwGetWindowSize(g_window, &w, &h);
  _arcball = new Arcball(w, h);

  return true;
}

//----------------------------------------------------------------------------------
void CityGen::Update()
{
  ImGuiIO& io = ImGui::GetIO();
  mousePressed[0] = mousePressed[1] = false;
  io.MouseWheel = 0;
  glfwPollEvents();
  UpdateImGui();
}

struct SecVertex
{
  vec3 pos;
};

struct SecEdge
{
  SecEdge(SecVertex* a, SecVertex* b) : a(a), b(b), dir(normalize(b->pos - a->pos)), len(distance(a->pos, b->pos)) {}
  SecVertex* a;
  SecVertex* b;
  vec3 dir;
  float len;
};

struct SecNode
{
  SecVertex* vtx;
  Tri* tri;
  vec3 pos;
  vec3 dir;
};

struct SecGraph
{
  SecGraph(u32 numVerts, u32 numEdges)
  {
    verts.resize(numVerts);
    edges.resize(numEdges);
  }

  ~SecGraph()
  {
    ContainerDelete(&verts);
    ContainerDelete(&edges);
  }

  vector<SecVertex*> verts;
  vector<SecEdge*> edges;
};


#define PARAM(name) GaussianRand(params.name, params.name * params.name ## Deviation)

struct SnapEvent
{
  enum class Type
  {
    NoSnap,
    NodeSnap,
    RoadSnap,
  };
  
  SnapEvent() : type(Type::NoSnap) {}
  SnapEvent(Type type, const vec3& pos) : type(type), vtx(nullptr), pos(pos) {}
  SnapEvent(Type type, SecVertex* vtx) : type(type), vtx(vtx), pos(vtx->pos) {}

  Type type;
  SecVertex* vtx;
  vec3 pos;
};

SnapEvent SnapNode(
    const SecGraph& graph,
    const vec3& a,
    const vec3& b,
    const vec3& dir,
    const CellParameterSet& params);


bool LinePointIntersect(const vec3& a, const vec3& b, const vec3& n, const vec3& p, float radius)
{
  // http://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line

  // determine if @p is within @radius of the line segment @a -> @b.
  vec3 ap = p - a;
  vec3 bp = p - b;

  // check if the intersection point is within the segment
  if (dot(n, ap) < 0)
    return false;

  if (dot(n, bp) > 0)
  {
    // the intersection pt is above the end node, so we want to
    // do a pt/pt distance check here
    return false;
  }

  // calc projection of ap onto ab
  vec3 proj = dot(ap, n) * n;

  // calc vector between projection and p
  vec3 dist = ap - proj;
  float len = length(dist);
  return len <= radius;
}

bool LineIntersect(const vec3& a, const vec3& b, const vec3& c, const vec3& d, vec3* out)
{
  // from: http://www-cs.ccny.cuny.edu/~wolberg/capstone/intersection/Intersection%20point%20of%20two%20lines.html
  float x1 = a.x; float x2 = b.x; float x3 = c.x; float x4 = d.x;
  float y1 = a.y; float y2 = b.y; float y3 = c.y; float y4 = d.y;
  float z1 = a.z; float z2 = b.z; float z3 = c.z; float z4 = d.z;

  float numx  = (x4 - x3) * (z1 - z3) - (z4 - z3) * (x1 - x3);
  float numz  = (x2 - x1) * (z1 - z3) - (z2 - z1) * (x1 - x3);
  float denom = (z4 - z3) * (x2 - x1) - (x4 - x3) * (z2 - z1);

  float ua = numx / denom;
  if (ua < 0 || ua > 1)
    return false;

  float ub = numz / denom;
  if (ub < 0 || ub > 1)
    return false;

  out->x = x1 + ua * (x2 - x1);
  out->y = y1 + ua * (y2 - y1);
  out->z = z1 + ua * (z2 - z1);

  return true;
}

SnapEvent SnapNode(
    const SecGraph& graph,
    SecVertex* org,
    const vec3& a,
    const vec3& b,
    const vec3& dir,
    const CellParameterSet& params)
{
  // test 1: check if the proposed segment is within snap distance to any existing vertex
  for (SecVertex* v : graph.verts)
  {
    if (org != v && LinePointIntersect(a, b, dir, v->pos, params.snapSize))
    {
      return SnapEvent(SnapEvent::Type::NodeSnap, v);
    }
  }

  // test 2: check if the proposed segment intersects any existing segment

  for (SecEdge* e : graph.edges)
  {
    vec3 res;
    if (org != e->a && org != e->b && LineIntersect(a, b, e->a->pos, e->b->pos, &res))
    {
      return SnapEvent(SnapEvent::Type::RoadSnap, res);
    }
  }

  return SnapEvent();
}

float SafeLen(const vec3& d)
{
  return sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
}

void PlaceSegment(
    SecVertex* org,
    const vec3& pos,
    const vec3& dir,
    const CellParameterSet& params,
    SecGraph* graph,
    deque<SecNode>* nodes)
{
  vec3 newPos = pos + PARAM(segmentSize) * dir;
  vec3 snapPos;

  // perform snap checking on the new pos.
  SnapEvent snap = SnapNode(*graph, org, pos, newPos, dir, params);
  switch (snap.type)
  {
    case SnapEvent::Type::NoSnap:
    {
      // add the node as-is
      SecVertex* v = new SecVertex{newPos};
      nodes->push_back(SecNode{v, g_terrain.FindTri(newPos), newPos, dir });
      graph->verts.push_back(v);
      graph->edges.push_back(new SecEdge{org, v});
      g_city._debugLines.push_back(pos);
      g_city._debugLines.push_back(newPos);
      break;
    }

    case SnapEvent::Type::RoadSnap:
    {
      // the new position intersects a road, so add a node at the intersection, and add
      // a new segment between newPos and the intersection
      if (randf(0, 1) < params.connectivity)
      {
        // add node
        newPos = snap.pos;
        if (SafeLen(newPos - pos) > 0.01f)
        {
          SecVertex* v = new SecVertex{newPos};
//          vec3 newDir = normalize(newPos - pos);
          //nodes->push_back(SecNode{ v, CITYGEN._terrain.FindTri(newPos), newPos, newDir });
          graph->verts.push_back(v);
          graph->edges.push_back(new SecEdge{org, v});
          g_city._debugLines.push_back(pos);
          g_city._debugLines.push_back(newPos);
        }
      }
      break;
    }

    case SnapEvent::Type::NodeSnap:
    {
      // new pos is close enough to an existing node to snap them together
      if (randf(0, 1) < params.connectivity)
      {
        // add node
        newPos = snap.vtx->pos;
        if (SafeLen(newPos - pos) > 0.01f)
        {
          vec3 newDir = normalize(newPos - pos);
          nodes->push_back(SecNode{snap.vtx, g_terrain.FindTri(newPos), newPos, newDir});
          graph->edges.push_back(new SecEdge{org, snap.vtx});
          g_city._debugLines.push_back(pos);
          g_city._debugLines.push_back(newPos);
        }
      }
      break;
    }
  }

}


//----------------------------------------------------------------------------------
void CityGen::CalcSecondary(const Cycle& cycle, const CellParameterSet& params)
{
  const vector<const Vertex*>& cverts = cycle.verts;
  if (cverts.size() < 2)
    return;

  SecGraph graph((u32)cverts.size(), (u32)cverts.size());
  vector<SecVertex*>& verts = graph.verts;
  vector<SecEdge*>& edges   = graph.edges;

  // create verts and edges
  for (size_t i = 0; i < cverts.size(); ++i)
  {
    const Vertex* cv = cverts[i];
    verts[i] = new SecVertex{cv->pos};
  }

  for (size_t i = 0; i < cverts.size(); ++i)
  {
    SecVertex* a  = verts[i];
    SecVertex* b  = verts[(i+1) % verts.size()];
    edges[i]      = new SecEdge(a, b);
  }

  // sort edges by length
  sort(edges.begin(), edges.end(), [](const SecEdge* lhs, const SecEdge* rhs) { return lhs->len > rhs->len; });

  deque<SecNode> nodes;

  // calc initial road segments
  for (size_t i = 0; i < edges.size(); ++i)
  {
    SecEdge* edge = edges[i];
    SecVertex* a  = edge->a;
    SecVertex* b  = edge->b;

    // calc deviated midpoint
    vec3 pt = lerp(a->pos, b->pos, GaussianRand(0.5f, 0.5f));
    Tri* tri = g_terrain.FindTri(pt, &pt);

    // calc road dir
    // this should lie in the arc in the plane described by the current triangle normal and the edge
    vec3 dir = cross(tri->n, edge->dir);
    vec3 newNode = pt + PARAM(segmentSize) * dir;

    SecVertex* v = new SecVertex{};

    nodes.push_back(SecNode{v, g_terrain.FindTri(newNode), newNode, dir});

    _debugLines.push_back(pt);
    _debugLines.push_back(newNode);
  }

  while (!nodes.empty() && _debugLines.size() < 1000)
  {
    SecNode node = FrontPop(nodes);
    float angle = -20;
    float angleInc = 2 * fabs(angle) / (params.degree - 1);
    vec3 dir = node.dir;
    vec3 pos = node.pos;
    Tri* t = node.tri;
    mat4 rot;
    mat4 id;

    for (int i = 0; i < params.degree; ++i)
    {
      // create rotation mtx around the triangle normal
      rot = glm::rotate(id, angle, vec3(0,1,0));
      angle += angleInc;
      vec4 rr(dir.x, dir.y, dir.z, 0);
      rr = rot * rr;
      vec3 newDir(rr.x, rr.y, rr.z);

      PlaceSegment(node.vtx, pos, newDir, params, &graph, &nodes);
    }
  }
}

#undef PARAM

#if 0
// calculate initial road segments
for each boundaryRoad in longest(boundaryRoads)
  midPoint = calculate deviated road midpoint
  sourceNode = insertNode(boundaryRoad, midPoint)
  roadDirection = calculate deviated boundaryRoad perpendicular
  if placeSegment(sourceNode, roadDirection, ref newNode)
    nodeQueue.push(newNode, roadDirection)

// process road growth
while (nodeQueue is not empty)
  sourceNode = nodeQueue.front().node;
  roadDirection = nodeQueue.front().direction;
  nodeQueue.pop();

  for ParamDegree iterations
    newDirection = rotate roadDirection by i*(360o / ParamDegree)
    deviate newDirection
    if placeSegment(sourceNode, newDirection, ref newNode)
      nodeQueue.push(newNode, newDirection)

// function to place road segment, returns true on success
placeSegment(sourceNode, roadDirection, ref newNode)
  roadLength = calculate deviated ParamSegmentLength
  targetPos = sourceNode.pos + (roadDirection * roadLength)

  switch (snapInformation(ParamSnapSize, sourceNode, targetPos))

  case: no snap event
        // insert node as-is
      targetNode = createNode(targetPos, roadLength)
      newNode = createRoad(sourceNode, targetNode)
      return true

  case: road snap event
        // node intersects existing road, so create node at intersection, and add new road
      if random value is less than ParamConectivity
        snapNode = insertNode(snapRoad, snapPoint)
        newNode = createRoad(sourceNode, snapNode)
        return true

  case: node snap event
        // node intersects existing node, so connect the two
      if random value is less than ParamConectivity
        newNode = createRoad(sourceNode, snapNode)
        return true
#endif

//----------------------------------------------------------------------------------
void CityGen::CalcCells()
{
  srand(1337);
    _debugLines.clear();

  vector<Cycle> cycles;
  _graph.CalcCycles(&cycles);

  for (const Cycle& cycle : cycles)
  {
    CalcSecondary(cycle, _cellParamterSets.back());
  }
}

//----------------------------------------------------------------------------------
void CityGen::AddPoint(const vec3& pt)
{
  ImGuiIO& io = ImGui::GetIO();
  if (io.KeyShift)
  {
    if (_clickFlags.IsSet(ClickFlagsF::FirstClick))
    {
      _dragStart = pt;
      _clickFlags.Clear(ClickFlagsF::FirstClick);
      _clickFlags.Set(ClickFlagsF::Dragging);
    }
    else
    {
      // second click, so add an edge

      // create vertices for the points, or get existing ones, if they exist
      Vertex* v0 = _graph.FindOrCreateVertex(_dragStart);
      Vertex* v1 = _graph.FindOrCreateVertex(pt);

      // only add non-degenerate edges
      if (v0 != v1)
      {
        _graph.AddEdge(v0, v1);

        // IMPORTANT: we save the snapped coords from the vertex, and not the actual input points
        // don't add the start point if it's the same as the previous drags point
        if (_nodes.empty() || _nodes.back() != v0->pos)
          _nodes.push_back(v0->pos);
        _nodes.push_back(v1->pos);
        GeneratePrimary();
      }
      _clickFlags.Clear(ClickFlagsF::Dragging);
      _clickFlags.Set(ClickFlagsF::FirstClick);
    }
  }
}

//----------------------------------------------------------------------------------
struct Stepper
{
  Stepper(StepSettings* settings, const vec3& start, const vec3& end)
      : settings(settings)
      , cur(start)
      , end(end)
  {
    vec3 dir = normalize(end - cur);
    angle = atan2(dir.z, dir.x);
  }

  vec3 Step(const vec3& goal)
  {
    // generate possible targets
    float ns = (float)settings->numSegments;
    float a = angle - settings->deviation / (ns-1);
    float s = 2 * settings->deviation / ns;

    float closest = FLT_MAX;
    vec3 nextStep;

    for (int i = 0; i < settings->numSegments; ++i)
    {
      vec3 pt = cur + settings->stepSize * vec3(cosf(a), 0, sinf(a));

      g_terrain.FindTri(pt, &pt);
      float tmp = length(goal - pt);
      if (tmp < closest)
      {
        closest = tmp;
        nextStep = pt;
        angle = a;
      }
      a += s;
    }

    cur = nextStep;
    return cur;
  }

  StepSettings *settings;
  vec3 cur, end;
  float angle;
};

//----------------------------------------------------------------------------------
void CityGen::GeneratePrimary()
{
  if (_nodes.size() < 2)
    return;

  _primary.clear();
  _debugLines.clear();

  deque<vec3> backward;
  vector<vec3> forward;

  for (int curIdx = 0; curIdx < _nodes.size() - 1; ++curIdx)
  {
    backward.clear();
    forward.clear();

    vec3 cur(_nodes[curIdx+0]);
    vec3 end(_nodes[curIdx+1]);

    // make 2 steppers, one for each direction
    Stepper forwardStepper(&_stepSettings, cur, end);
    Stepper backwardStepper(&_stepSettings, end, cur);

    forward.push_back(cur);
    backward.push_back(end);

    float prevLen = FLT_MAX;
    vec3 f = cur;
    vec3 b = end;

    while (true)
    {
      f = forwardStepper.Step(b);
      b = backwardStepper.Step(f);

      forward.push_back(f);
      backward.push_front(b);

      float len = distance(f, b);
      if (len <= g_terrain._scale || len >= prevLen)
      {
        for (const vec3& v : forward)
          _primary.push_back(v);

        for (const vec3& v : backward)
          _primary.push_back(v);

        break;
      }
      prevLen = len;
    }

  }
}

//----------------------------------------------------------------------------------
void CityGen::RenderUI()
{
  static bool fileMenuOpen = true;
  ImGui::Begin("File", &fileMenuOpen, ImVec2(200, 100));

  if (ImGui::Button("Calc cells"))
  {
    CalcCells();
  }

  if (ImGui::Button("Load settings"))
  {
    LoadSettings(_configFile.c_str());
  }

  if (ImGui::Button("Save settings"))
  {
    SaveSettings(_configFile.c_str());
  }

  ImGui::End();

  static bool open = true;
  ImGui::Begin("Properties", &open, ImVec2(200, 100));

  if (ImGui::InputFloat("scale", &g_terrain._scale, 1)
    || ImGui::InputFloat("h-scale", &g_terrain._heightScale, 0.1f))
  {
    g_terrain._scale = max(1.f, min(100.f, g_terrain._scale));
    g_terrain._heightScale = min(5.f, g_terrain._heightScale);
    g_terrain.CreateMesh();
  }

  if (ImGui::InputInt("# segments", &_stepSettings.numSegments, 1, 5)
    || ImGui::InputFloat("sample size", &_stepSettings.stepSize, 0.1f, 5)
    || ImGui::InputFloat("deviation", &_stepSettings.deviation, DEG_TO_RAD(1))
    || ImGui::InputFloat("deviation", &_stepSettings.roadHeight, 1))
  {
    GeneratePrimary();
  }

  ImGui::Separator();

  CellParameterSet& cell = _cellParamterSets.front();

  if (ImGui::InputFloat("segment_size", &cell.segmentSize, 1, 5) || ImGui::InputFloat("deviation", &cell.segmentSizeDeviation, 0.1f, 1)
      || ImGui::InputInt("degree", &cell.degree)
      || ImGui::InputFloat("snap_size", &cell.snapSize, 1, 5) || ImGui::InputFloat("deviation", &cell.snapSizeDeviation, 0.1f, 1)
      || ImGui::InputFloat("connectivity", &cell.connectivity, 0.05f, 5))
  {
    CalcCells();
  }

  ImGui::Separator();
  ImGui::Checkbox("normals", &_drawNormals);
  ImGui::Checkbox("debuglines", &_drawDebugLines);

  ImGui::End();
}

//----------------------------------------------------------------------------------
void CityGen::Render()
{
  ImGuiIO& io = ImGui::GetIO();

  // Rendering
  glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
  glClearColor(0.8f, 0.6f, 0.6f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  RenderUI();

  // calc proj and view matrix
  glMatrixMode(GL_PROJECTION);
  g_proj = glm::perspective(60.0f, 1.333f, 1.f, 10000.0f);
  glLoadMatrixf(glm::value_ptr(g_proj));

  float z = 1150;
  glMatrixMode(GL_MODELVIEW);
  g_cameraPos = glm::vec3(0.0f, 600, z);
//  g_cameraPos = glm::vec3(0.0f, 500, 1500);
  mat4 dir = glm::lookAt(g_cameraPos, glm::vec3(0, 0, z), glm::vec3(0, 0, -1) );
  _rot = g_city._arcball->createViewRotationMatrix();
  g_view = _rot * dir;
  glLoadMatrixf(glm::value_ptr(g_view));

  glEnable(GL_LINE_SMOOTH);
  glLineWidth(1.2f);

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  glEnableClientState(GL_VERTEX_ARRAY);

  // draw terrain
  glColor4ub(0xfd, 0xf6, 0xe3, 255);
  glVertexPointer(3, GL_FLOAT, 0, g_terrain._verts.data());
  glDrawElements(GL_TRIANGLES, (GLsizei)g_terrain._indices.size(), GL_UNSIGNED_INT, g_terrain._indices.data());

  if (_drawNormals)
  {
    vector<vec3> normals(g_terrain._tris.size()*2);
    for (size_t i = 0; i < g_terrain._tris.size(); ++i)
    {
      const Tri& tri = g_terrain._tris[i];
      vec3 p = (tri.v0 + tri.v1 + tri.v2) / 3.f;
      normals[i*2+0] = p;
      normals[i*2+1] = p + 10.f * tri.n;
    }

    glColor4ub(0x0, 0xfd, 0x0, 255);
    glVertexPointer(3, GL_FLOAT, 0, normals.data());
    glDrawArrays(GL_LINES, 0, (GLsizei)normals.size());
  }

  if (_drawDebugLines && !_debugLines.empty())
  {
    glColor4ub(0x0, 0xfd, 0x0, 255);
    glVertexPointer(3, GL_FLOAT, 0, _debugLines.data());
    glDrawArrays(GL_LINES, 0, (GLsizei)_debugLines.size());
  }

  // draw intersected tris
  if (!g_terrain._intersected.empty())
  {
    glColor4ub(0xfd, 0xf6, 0x0, 255);
    glVertexPointer(3, GL_FLOAT, 0, g_terrain._intersected.data());
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)g_terrain._intersected.size());
  }

  // draw primary
  if (_primary.size() >= 2)
  {
    glColor4ub(0xfd, 0x0, 0x0, 255);
    glVertexPointer(3, GL_FLOAT, 0, _primary.data());
    glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)_primary.size());
  }

  glDisableClientState(GL_VERTEX_ARRAY);

  ImGui::Render();
  glfwSwapBuffers(g_window);
}

//----------------------------------------------------------------------------------
bool CityGen::Run()
{
  while (!glfwWindowShouldClose(g_window))
  {
    Update();
    Render();
  }

  return true;
}

//----------------------------------------------------------------------------------
bool CityGen::Close()
{
  ImGui::Shutdown();
  glfwTerminate();

  return true;
}

//----------------------------------------------------------------------------------
void CityGen::LoadSettings(const char* filename)
{
  protocol::City city;
  if (!LoadProto(filename, &city))
    return;

  // load settings
  protocol::Settings settings = city.settings();

  g_terrain.FromProtocol(settings.terrain_settings());

  _stepSettings.FromProtocol(settings.step_settings());

  _cellParamterSets.clear();
  for (const auto& ps : settings.cell_parameter_set())
  {
    _cellParamterSets.push_back(CellParameterSet());
    _cellParamterSets.back().FromProtocol(ps);
  }

  // load nodes
  _nodes.clear();
  _nodes.reserve(city.nodes_size());
  for (const protocol::Vector3& v : city.nodes())
  {
    _nodes.push_back(FromProtocol(v));
  }
}

//----------------------------------------------------------------------------------
void CityGen::SaveSettings(const char* filename)
{
  protocol::City city;
  protocol::Settings* settings = city.mutable_settings();

  g_terrain.ToProtocol(settings->mutable_terrain_settings());
  _stepSettings.ToProtocol(settings->mutable_step_settings());

  for (const auto& ps : _cellParamterSets)
    ps.ToProtocol(settings->add_cell_parameter_set());

  for (const vec3& v : _nodes)
  {
    ToProtocol(v, city.add_nodes());
  }

  string str = city.DebugString();
  if (FILE* f = fopen(filename, "wt"))
  {
    fputs(str.c_str(), f);
    fclose(f);
  }
}

//----------------------------------------------------------------------------------
void CityGen::PostLoadSetup()
{
  g_terrain.CreateMesh();

  // add a default parameter set
  if (_cellParamterSets.empty())
  {
    _cellParamterSets.push_back(CellParameterSet());
  }

  _graph.Reset();

  if (!_nodes.empty())
  {
    // create the graph, and create the primary nodes
    for (size_t i = 0; i < _nodes.size() - 1; ++i)
    {
      Vertex* v0 = _graph.FindOrCreateVertex(_nodes[i+0]);
      Vertex* v1 = _graph.FindOrCreateVertex(_nodes[i+1]);
      _graph.AddEdge(v0, v1);
    }
    GeneratePrimary();
  }

}

//----------------------------------------------------------------------------------
int main(int argc, char** argv)
{
  if (!g_city.Init())
    return 1;

  g_city.Run();

  g_city.Close();

  return 0;
}
