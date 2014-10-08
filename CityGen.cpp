#include "citygen.hpp"
#include "imgui_helpers.hpp"
#include "utils.hpp"
#pragma warning(push)
#pragma warning(disable: 4244 4267)
#include "protocol/city.pb.h"
#pragma warning(pop)

using namespace citygen;
CityGen* CityGen::_instance;

//----------------------------------------------------------------------------------
CityGen::CityGen()
    : _arcball(nullptr)
    , _drawDebugLines(true)
    , _drawNormals(false)
{
  _stateFlags.Set(StateFlagsF::Paused);
  _clickFlags.Set(ClickFlagsF::FirstClick);
}

//----------------------------------------------------------------------------------
void CityGen::Create()
{
  if (!_instance)
  {
    _instance = new CityGen();
  }
}

//----------------------------------------------------------------------------------
void CityGen::Destroy()
{
  if (_instance)
  {
    delete exch_null(_instance);
  }
}

//----------------------------------------------------------------------------------
CityGen& CityGen::Instance()
{
  return *_instance;
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


  #if _WIN32
  string base = "/projects/citygen/";
  #else
  string base = "/Users/dooz/projects/citygen/";
  #endif

  _configFile = base + "config/city1.pb";
  _terrain._data = stbi_load((base + "noise.tga").c_str(), &_terrain._w, &_terrain._h, &_terrain._depth, 0);
  _terrain.CreateMesh();

  LoadSettings(_configFile.c_str());

  InitGL();
  InitImGui();

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

//----------------------------------------------------------------------------------
void CityGen::CalcCells()
{
  _graph.CalcCycles();
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
      Vertex* v0 = _graph.FindOrCreateVertex(&_terrain, _dragStart);
      Vertex* v1 = _graph.FindOrCreateVertex(&_terrain, pt);

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
  Stepper(Terrain* terrain, StepSettings* settings, const vec3& start, const vec3& end)
      : terrain(terrain)
      , settings(settings)
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

      Tri* tri = terrain->FindTri(pt, &pt);
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

  Terrain* terrain;
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
    Stepper forwardStepper(&_terrain, &_stepSettings, cur, end);
    Stepper backwardStepper(&_terrain, &_stepSettings, end, cur);

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
      if (len <= _terrain._scale || len >= prevLen)
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

  if (ImGui::InputFloat("scale", &_terrain._scale, 1)
    || ImGui::InputFloat("h-scale", &_terrain._heightScale, 0.1f))
  {
    _terrain._scale = max(1.f, min(100.f, _terrain._scale));
    _terrain._heightScale = max(0.1f, min(5.f, _terrain._heightScale));
    _terrain.CreateMesh();
  }

  if (ImGui::InputInt("# segments", &_stepSettings.numSegments, 1, 5)
    || ImGui::InputFloat("sample size", &_stepSettings.stepSize, 0.1f, 5)
    || ImGui::InputFloat("deviation", &_stepSettings.deviation, DEG_TO_RAD(1))
    || ImGui::InputFloat("deviation", &_stepSettings.roadHeight, 1))
  {
    GeneratePrimary();
  }

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
  g_proj = glm::perspective(60.0f, 1.333f, 1.f, 2000.0f);
  glLoadMatrixf(glm::value_ptr(g_proj));

  glMatrixMode(GL_MODELVIEW);
  g_cameraPos = glm::vec3(0.0f, 30, 0);
  g_cameraPos = glm::vec3(0.0f, 500, 1500);
  mat4 dir = glm::lookAt(g_cameraPos, glm::vec3(0, 0, 0), glm::vec3(0, 0, -1) );
  _rot = CITYGEN._arcball->createViewRotationMatrix();
  g_view = _rot * dir;
  glLoadMatrixf(glm::value_ptr(g_view));

  glEnable(GL_LINE_SMOOTH);
  glLineWidth(1.2f);

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  glEnableClientState(GL_VERTEX_ARRAY);

  // draw terrain
  glColor4ub(0xfd, 0xf6, 0xe3, 255);
  glVertexPointer(3, GL_FLOAT, 0, _terrain._verts.data());
  glDrawElements(GL_TRIANGLES, (GLsizei)_terrain._indices.size(), GL_UNSIGNED_INT, _terrain._indices.data());

  if (_drawNormals)
  {
    vector<vec3> normals(_terrain._tris.size()*2);
    for (size_t i = 0; i < _terrain._tris.size(); ++i)
    {
      const Tri& tri = _terrain._tris[i];
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
  if (!_terrain._intersected.empty())
  {
    glColor4ub(0xfd, 0xf6, 0x0, 255);
    glVertexPointer(3, GL_FLOAT, 0, _terrain._intersected.data());
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)_terrain._intersected.size());
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
  protocol::Settings settings         = city.settings();
  protocol::StepSettings stepSettings = settings.step_settings();

  _stepSettings.numSegments           = stepSettings.num_segments();
  _stepSettings.stepSize              = stepSettings.step_size();
  _stepSettings.deviation             = stepSettings.deviation();
  _stepSettings.roadHeight            = stepSettings.road_height();

  // load nodes
  _nodes.clear();
  _nodes.reserve(city.nodes_size());
  for (const protocol::Vector3& v : city.nodes())
  {
    _nodes.push_back(FromProtocol(v));
  }

  _graph.Reset();

  // create the graph, and create the primary nodes
  for (size_t i = 0; i < _nodes.size() - 1; ++i)
  {
    Vertex* v0 = _graph.FindOrCreateVertex(&_terrain, _nodes[i+0]);
    Vertex* v1 = _graph.FindOrCreateVertex(&_terrain, _nodes[i+1]);
    _graph.AddEdge(v0, v1);
  }
  GeneratePrimary();

}

//----------------------------------------------------------------------------------
void CityGen::SaveSettings(const char* filename)
{
  protocol::City city;
  protocol::Settings* settings = city.mutable_settings();
  protocol::StepSettings* stepSettings = settings->mutable_step_settings();

  stepSettings->set_num_segments(_stepSettings.numSegments);
  stepSettings->set_step_size(_stepSettings.stepSize);
  stepSettings->set_deviation(_stepSettings.deviation);
  stepSettings->set_road_height(_stepSettings.roadHeight);

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
int main(int argc, char** argv)
{

  CityGen::Create();
  if (!CITYGEN.Init())
    return 1;

  CITYGEN.Run();

  CITYGEN.Close();

  CityGen::Destroy();

  return 0;
}
