#include "citygen.hpp"
#include "imgui_helpers.hpp"

using namespace citygen;
CityGen* CityGen::_instance;


//----------------------------------------------------------------------------------
void Terrain::CreateMesh()
{
  int w = _w;
  int h = _h;
  float scale = _scale;
  float heightScale = _heightScale;

  _indices.resize((w-1) * (h-1) * 2 * 3);
  _tris.resize((w-1) * (h-1) * 2);
  _verts.resize(w*h);
  vec3* vertPtr = _verts.data();
  u32* indPtr = _indices.data();
  u8* ptr = _data;

  _minValues.x = -(w-1)/2.f * scale;
  _minValues.z = -(h-1)/2.f * scale;
  _maxValues.x = +(w-1)/2.f * scale;
  _maxValues.z = +(h-1)/2.f * scale;

  // remeber, because negative z goes into the screen, the first triangle is the furtherst left corner
  float z = _minValues.z;
  for (int i = 0; i < h; ++i)
  {
    float x = _minValues.z;
    for (int j = 0; j < w; ++j)
    {
      float y = (ptr[0] + ptr[1] + ptr[2]) / 3.f * heightScale;

      *vertPtr++ = vec3(x, y, z);

      if (i < h-1 && j < w-1)
      {
        // note where the +1 offsets in y are due to the z-order
        // v1-v3
        // v0-v2
        u32 v0 = (i+1)*w+(j+0);
        u32 v1 = (i+0)*w+(j+0);
        u32 v2 = (i+1)*w+(j+1);
        u32 v3 = (i+0)*w+(j+1);

        indPtr[0] = v0;
        indPtr[1] = v1;
        indPtr[2] = v2;

        indPtr[3] = v2;
        indPtr[4] = v1;
        indPtr[5] = v3;

        indPtr += 6;
      }

      ptr += 4;
      x += scale;
    }
    z += scale;
  }

  // calc tris
  Tri* triPtr = _tris.data();
  for (int i = 0; i < h-1; ++i)
  {
    for (int j = 0; j < w - 1; ++j)
    {
      // v1-v3
      // v0-v2
      u32 i0 = (i+1)*w+(j+0);
      u32 i1 = (i+0)*w+(j+0);
      u32 i2 = (i+1)*w+(j+1);
      u32 i3 = (i+0)*w+(j+1);

      const vec3& v0 = _verts[i0];
      const vec3& v1 = _verts[i1];
      const vec3& v2 = _verts[i2];
      const vec3& v3 = _verts[i3];

      triPtr->v0 = v0;
      triPtr->v1 = v1;
      triPtr->v2 = v2;
      triPtr->n = glm::normalize(glm::cross(v1-v0, v2-v0));
      triPtr++;

      triPtr->v0 = v2;
      triPtr->v1 = v1;
      triPtr->v2 = v3;
      triPtr->n = glm::normalize(glm::cross(v2-v3, v1-v3));
      triPtr++;
    }
  }
}

//----------------------------------------------------------------------------------
void Terrain::CalcIntersection(const vec3& org, const vec3& dir)
{
  _intersected.clear();
  Tri closestTri;
  vec3 closestPos;
  float closest = FLT_MAX;
  for (size_t i = 0; i < _tris.size(); ++i)
  {
    const Tri& tri = _tris[i];
    // pos = [t, u, v]
    vec3 pos;
    if (glm::intersectLineTriangle(org, dir, tri.v0, tri.v1, tri.v2, pos) && pos.x < closest)
    {
      closest = pos.x;
      closestTri = tri;
      closestPos = pos;
    }
  }

  if (closest != FLT_MAX)
  {
    _intersected.push_back(closestTri.v0);
    _intersected.push_back(closestTri.v1);
    _intersected.push_back(closestTri.v2);
    float u = closestPos.y;
    float v = closestPos.z;
    _intersection = (1 - u - v) * closestTri.v0 + u * closestTri.v1 + v * closestTri.v2;
  }
}

//----------------------------------------------------------------------------------
Tri* Terrain::FindTri(const vec3& pt, vec3* out)
{
  // Note, we project to x/z plane, so just ignore the y-component
  if (pt.x < _minValues.x || pt.x > _maxValues.x || pt.z < _minValues.z || pt.z > _maxValues.z)
    return nullptr;

  vec3 ofs = pt - _minValues;

  // find the quad the point is in
  int x = (int)(ofs.x / _scale);
  int z = (int)(ofs.z / _scale);

  // find the barycentric coords
  //  v1 v3
  //  v0 v2
  // P = w*v0 + v*v1 + u*v2
  float u = (ofs.x - x * _scale) / _scale;
  float v = 1 - (ofs.z - z * _scale) / _scale;
  float w = (1 - u - v);

  // if w is within [0, 1] then we are inside the bottom left triangle
  if ( w >= 0 && w <= 1)
  {
//    printf("u, %f, v: %f, w: %f\n", u, v, w);
    Tri* t = &_tris[2 * (z * (_w-1) + x) + 0];
    *out = w * t->v0 + v * t->v1 + u * t->v2;
    return t;
  }
  else
  {
    // upper triangle.
    // negative u and v, so top right betcomes w
    Tri* t = &_tris[2 * (z * (_w-1) + x) + 1];
    u = 1 - u;
    v = 1 - v;
    w = (1 - u - v);
    *out = w * t->v2 + v * t->v0 + u * t->v1;
    return t;
  }
}

//----------------------------------------------------------------------------------
CityGen::CityGen()
    : _arcball(nullptr)
    , _drawDebugLines(true)
    , _drawNormals(false)
{
  _stateFlags.Set(StateFlagsF::Paused);
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
  _terrain._data = stbi_load("/projects/citygen/noise.tga", &_terrain._w, &_terrain._h, &_terrain._depth, 0);
#else
  _terrain._data = stbi_load("/Users/dooz/projects/citygen/noise.tga", &_terrain._w, &_terrain._h, &_terrain._depth, 0);
#endif
  _terrain.CreateMesh();

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
void CityGen::AddPoint(const vec3& pt)
{
  ImGuiIO& io = ImGui::GetIO();
  if (io.KeyShift)
  {
    _points.push_back(pt);
    GeneratePrimary();
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
  if (_points.size() < 2)
    return;

  _primary.clear();
  _debugLines.clear();

  deque<vec3> backward;
  vector<vec3> forward;

  for (int curIdx = 0; curIdx < _points.size() - 1; ++curIdx)
  {
    backward.clear();
    forward.clear();

    vec3 cur(_points[curIdx+0]);
    vec3 end(_points[curIdx+1]);

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
  if (ImGui::Button("Load settings"))
  {

  }

  if (ImGui::Button("Save settings"))
  {

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

}

//----------------------------------------------------------------------------------
void CityGen::SaveSettings(const char* filename)
{

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
