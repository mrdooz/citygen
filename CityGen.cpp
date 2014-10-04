#include "citygen.hpp"

using namespace citygen;
CityGen* CityGen::_instance;

#ifdef _MSC_VER
#pragma warning (disable: 4996)         // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#include <Windows.h>
#include <Imm.h>
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "imm32.lib")
#if _DEBUG
#pragma comment(lib, "/projects/glfw/src/debug/glfw3.lib")
#else
#pragma comment(lib, "/projects/glfw/src/release/glfw3.lib")
#endif
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "imgui/stb_image.h"                  // for .png loading
#include "imgui/imgui.h"

#include "arcball.hpp"
#include "glm/gtx/intersect.hpp"

static GLFWwindow* g_window;
static int g_windowWidth = 1280;
static int g_windowHeight = 720;
mat4 g_view, g_proj;
vec3 g_cameraPos;

static GLuint fontTex;
static bool mousePressed[2] = { false, false };
static ImVec2 mousePosScale(1.0f, 1.0f);

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
// - try adjusting ImGui::GetIO().PixelCenterOffset to 0.5f or 0.375f
static void ImImpl_RenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count)
{
  if (cmd_lists_count == 0)
    return;

  // We are using the OpenGL fixed pipeline to make the example code simpler to read!
  // A probable faster way to render would be to collate all vertices from all cmd_lists into a single vertex buffer.
  // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers.
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  // Setup texture
  glBindTexture(GL_TEXTURE_2D, fontTex);
  glEnable(GL_TEXTURE_2D);

  // Setup orthographic projection matrix
  const float width = ImGui::GetIO().DisplaySize.x;
  const float height = ImGui::GetIO().DisplaySize.y;
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0f, width, height, 0.0f, -1.0f, +1.0f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Render command lists
  for (int n = 0; n < cmd_lists_count; n++)
  {
    const ImDrawList* cmd_list = cmd_lists[n];
    const unsigned char* vtx_buffer = (const unsigned char*)cmd_list->vtx_buffer.begin();
    glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer));
    glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer+8));
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (void*)(vtx_buffer+16));

    int vtx_offset = 0;
    const ImDrawCmd* pcmd_end = cmd_list->commands.end();
    for (const ImDrawCmd* pcmd = cmd_list->commands.begin(); pcmd != pcmd_end; pcmd++)
    {
      glScissor((int)pcmd->clip_rect.x, (int)(height - pcmd->clip_rect.w), (int)(pcmd->clip_rect.z - pcmd->clip_rect.x), (int)(pcmd->clip_rect.w - pcmd->clip_rect.y));
      glDrawArrays(GL_TRIANGLES, vtx_offset, pcmd->vtx_count);
      vtx_offset += pcmd->vtx_count;
    }
  }
  glDisable(GL_SCISSOR_TEST);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);
}

// NB: ImGui already provide OS clipboard support for Windows so this isn't needed if you are using Windows only.
static const char* ImImpl_GetClipboardTextFn()
{
  return glfwGetClipboardString(g_window);
}

static void ImImpl_SetClipboardTextFn(const char* text)
{
  glfwSetClipboardString(g_window, text);
}

#ifdef _MSC_VER
// Notify OS Input Method Editor of text input position (e.g. when using Japanese/Chinese inputs, otherwise this isn't needed)
static void ImImpl_ImeSetInputScreenPosFn(int x, int y)
{
    HWND hwnd = glfwGetWin32Window(g_window);
    if (HIMC himc = ImmGetContext(hwnd))
    {
        COMPOSITIONFORM cf;
        cf.ptCurrentPos.x = x;
        cf.ptCurrentPos.y = y;
        cf.dwStyle = CFS_FORCE_POSITION;
        ImmSetCompositionWindow(himc, &cf);
    }
}
#endif

// GLFW callbacks to get events
static void glfw_error_callback(int error, const char* description)
{
  fputs(description, stderr);
}

static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  CITYGEN._arcball->mouseButtonCallback( window, button, action, mods );

  if (action == GLFW_PRESS && button >= 0 && button < 2)
    mousePressed[button] = true;

  if (action == GLFW_RELEASE && button == 0)
  {
    CITYGEN.AddPoint(CITYGEN._terrain._intersection);
  }
}

static void glfw_cursor_callback(GLFWwindow *window, double x, double y)
{
  CITYGEN._arcball->cursorCallback(g_window, x, y);

  float depth = 1;
  glm::vec4 viewport = glm::vec4(0, 0, g_windowWidth, g_windowHeight);
  glm::vec3 wincoord = glm::vec3(x, g_windowHeight - y - 1, depth);
  glm::vec3 objcoord = glm::unProject(wincoord, g_view, g_proj, viewport);

  vec3 dir = glm::normalize(objcoord - g_cameraPos);
  CITYGEN._terrain.CalcIntersection(g_cameraPos, dir);
}

static void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  ImGuiIO& io = ImGui::GetIO();
  io.MouseWheel = (yoffset != 0.0f) ? yoffset > 0.0f ? 1 : - 1 : 0;           // Mouse wheel: -1,0,+1
}

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  ImGuiIO& io = ImGui::GetIO();
  if (action == GLFW_PRESS)
    io.KeysDown[key] = true;
  if (action == GLFW_RELEASE)
    io.KeysDown[key] = false;
  io.KeyCtrl = (mods & GLFW_MOD_CONTROL) != 0;
  io.KeyShift = (mods & GLFW_MOD_SHIFT) != 0;
}

static void glfw_char_callback(GLFWwindow* window, unsigned int c)
{
  if (c > 0 && c < 0x10000)
    ImGui::GetIO().AddInputCharacter((unsigned short)c);
}

// OpenGL code based on http://open.gl tutorials
void InitGL()
{
  glfwSetErrorCallback(glfw_error_callback);

  if (!glfwInit())
    exit(1);

  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  g_window = glfwCreateWindow(g_windowWidth, g_windowHeight, "ImGui OpenGL example", NULL, NULL);
  glfwMakeContextCurrent(g_window);
  glfwSetKeyCallback(g_window, glfw_key_callback);
  glfwSetMouseButtonCallback(g_window, glfw_mouse_button_callback);
  glfwSetScrollCallback(g_window, glfw_scroll_callback);
  glfwSetCharCallback(g_window, glfw_char_callback);
  glfwSetCursorPosCallback(g_window, glfw_cursor_callback);

  //glewInit();

  int w, h;
  glfwGetWindowSize(g_window, &w, &h);
  CITYGEN._arcball = new Arcball(w, h);
}

void InitImGui()
{
  int w, h;
  int fb_w, fb_h;
  glfwGetWindowSize(g_window, &w, &h);
  glfwGetFramebufferSize(g_window, &fb_w, &fb_h);
  mousePosScale.x = (float)fb_w / w;                  // Some screens e.g. Retina display have framebuffer size != from window size, and mouse inputs are given in window/screen coordinates.
  mousePosScale.y = (float)fb_h / h;

  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize = ImVec2((float)fb_w, (float)fb_h);  // Display size, in pixels. For clamping windows positions.
  io.DeltaTime = 1.0f/60.0f;                          // Time elapsed since last frame, in seconds (in this sample app we'll override this every frame because our timestep is variable)
  io.PixelCenterOffset = 0.0f;                        // Align OpenGL texels
  io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;             // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
  io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
  io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
  io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
  io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
  io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
  io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
  io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
  io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
  io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
  io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
  io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
  io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
  io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

  io.RenderDrawListsFn = ImImpl_RenderDrawLists;
  io.SetClipboardTextFn = ImImpl_SetClipboardTextFn;
  io.GetClipboardTextFn = ImImpl_GetClipboardTextFn;
#ifdef _MSC_VER
    io.ImeSetInputScreenPosFn = ImImpl_ImeSetInputScreenPosFn;
#endif

  // Load font texture
  glGenTextures(1, &fontTex);
  glBindTexture(GL_TEXTURE_2D, fontTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

#if 1
  // Default font (embedded in code)
  const void* png_data;
  unsigned int png_size;
  ImGui::GetDefaultFontData(NULL, NULL, &png_data, &png_size);
  int tex_x, tex_y, tex_comp;
  void* tex_data = stbi_load_from_memory((const unsigned char*)png_data, (int)png_size, &tex_x, &tex_y, &tex_comp, 0);
  IM_ASSERT(tex_data != NULL);
#else
    // Custom font from filesystem
    io.Font = new ImBitmapFont();
    io.Font->LoadFromFile("../../extra_fonts/mplus-2m-medium_18.fnt");
    IM_ASSERT(io.Font->IsLoaded());

    int tex_x, tex_y, tex_comp;
    void* tex_data = stbi_load("../../extra_fonts/mplus-2m-medium_18.png", &tex_x, &tex_y, &tex_comp, 0);
    IM_ASSERT(tex_data != NULL);

    // Automatically find white pixel from the texture we just loaded
    // (io.FontTexUvForWhite needs to contains UV coordinates pointing to a white pixel in order to render solid objects)
    for (int tex_data_off = 0; tex_data_off < tex_x*tex_y; tex_data_off++)
        if (((unsigned int*)tex_data)[tex_data_off] == 0xffffffff)
        {
            io.FontTexUvForWhite = ImVec2((float)(tex_data_off % tex_x)/(tex_x), (float)(tex_data_off / tex_x)/(tex_y));
            break;
        }
#endif

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_x, tex_y, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);
  stbi_image_free(tex_data);
}

void UpdateImGui()
{
  ImGuiIO& io = ImGui::GetIO();

  // Setup timestep
  static double time = 0.0f;
  const double current_time =  glfwGetTime();
  io.DeltaTime = (float)(current_time - time);
  time = current_time;

  // Setup inputs
  // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
  double mouse_x, mouse_y;
  glfwGetCursorPos(g_window, &mouse_x, &mouse_y);

  io.MousePos = ImVec2((float)mouse_x * mousePosScale.x, (float)mouse_y * mousePosScale.y);      // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
  io.MouseDown[0] = mousePressed[0] || glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_LEFT) != 0;  // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
  io.MouseDown[1] = mousePressed[1] || glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_RIGHT) != 0;

  // Start the frame
  ImGui::NewFrame();
}

//----------------------------------------------------------------------------------
Terrain::Terrain()
{
  memset(this, 0, sizeof(Terrain));
  _scale = 20;
  _heightScale = 1;
}

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
  Stepper(Terrain* terrain, const vec3& start, const vec3& end, float stepSize, float deviation, int numSegments)
      : terrain(terrain)
      , cur(start)
      , end(end)
      , stepSize(stepSize)
      , deviation(deviation)
      , numSegments(numSegments)
  {
    vec3 dir = normalize(end - cur);
    angle = atan2(dir.z, dir.x);
  }

  vec3 Step(const vec3& goal)
  {
    // generate possible targets
    float ns = (float)numSegments;
    float a = angle - deviation / (ns-1);
    float s = 2 * deviation / ns;

    float closest = FLT_MAX;
    vec3 nextStep;

    for (int i = 0; i < numSegments; ++i)
    {
      vec3 pt = cur + stepSize * vec3(cosf(a), 0, sinf(a));

      Tri* tri = terrain->FindTri(pt, &pt);
      float tmp = length(goal - pt);
      //tmp = fabs(cur.y - pt2.y);
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
  vec3 cur, end;
  float stepSize;
  float deviation;
  int numSegments;
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
    Stepper forwardStepper(&_terrain, cur, end, _stepSize, _deviation, _numSegments);
    Stepper backwardStepper(&_terrain, end, cur, _stepSize, _deviation, _numSegments);

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
  static bool open = true;
  ImGui::Begin("Properties", &open, ImVec2(200, 100));

  if (ImGui::InputFloat("scale", &_terrain._scale, 1)
    || ImGui::InputFloat("h-scale", &_terrain._heightScale, 0.1f))
  {
    _terrain._scale = max(1.f, min(100.f, _terrain._scale));
    _terrain._heightScale = max(0.1f, min(5.f, _terrain._heightScale));
    _terrain.CreateMesh();
  }

  if (ImGui::InputInt("# segments", &_numSegments, 1, 5)
    || ImGui::InputFloat("sample size", &_stepSize, 0.1f, 5)
    || ImGui::InputFloat("deviation", &_deviation, DEG_TO_RAD(1)))
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
