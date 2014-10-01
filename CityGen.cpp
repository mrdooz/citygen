#include "citygen.hpp"

#include <stdio.h>

#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

static GLuint g_fontTex;
u32 g_width, g_height;

using namespace citygen;

CityGen* CityGen::_instance;

//----------------------------------------------------------------------------------
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
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, g_fontTex);

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

#if 0
static const char* ImImpl_GetClipboardTextFn()
{
  // TODO: magnus
  return nullptr;
  //return glfwGetClipboardString(window);
}

static void ImImpl_SetClipboardTextFn(const char* text, const char* text_end)
{
  // TODO: magnus
/*
  if (!text_end)
    text_end = text + strlen(text);

  if (*text_end == 0)
  {
    // Already got a zero-terminator at 'text_end', we don't need to add one
    glfwSetClipboardString(window, text);
  }
  else
  {
    // Add a zero-terminator because glfw function doesn't take a size
    char* buf = (char*)malloc(text_end - text + 1);
    memcpy(buf, text, text_end-text);
    buf[text_end-text] = '\0';
    glfwSetClipboardString(window, buf);
    free(buf);
  }
*/
}


// GLFW callbacks to get events
// static void glfw_error_callback(int error, const char* description)
// {
//   fputs(description, stderr);
// }
// 
// static void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
// {
//   ImGuiIO& io = ImGui::GetIO();
//   io.MouseWheel = (yoffset != 0.0f) ? yoffset > 0.0f ? 1 : -1 : 0;           // Mouse wheel: -1,0,+1
// }
// 
// static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
// {
//   ImGuiIO& io = ImGui::GetIO();
//   if (action == GLFW_PRESS)
//     io.KeysDown[key] = true;
//   if (action == GLFW_RELEASE)
//     io.KeysDown[key] = false;
//   io.KeyCtrl = (mods & GLFW_MOD_CONTROL) != 0;
//   io.KeyShift = (mods & GLFW_MOD_SHIFT) != 0;
// }
// 
// static void glfw_char_callback(GLFWwindow* window, unsigned int c)
// {
//   if (c > 0 && c <= 255)
//     ImGui::GetIO().AddInputCharacter((char)c);
// }
// 
// // OpenGL code based on http://open.gl tutorials
// void InitGL()
// {
//   glfwSetErrorCallback(glfw_error_callback);
// 
//   if (!glfwInit())
//     exit(1);
// 
// //   glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
// //   window = glfwCreateWindow(1280, 720, "ImGui OpenGL example", NULL, NULL);
// //   glfwMakeContextCurrent(window);
// //   glfwSetKeyCallback(window, glfw_key_callback);
// //   glfwSetScrollCallback(window, glfw_scroll_callback);
// //   glfwSetCharCallback(window, glfw_char_callback);
// // 
// //   glewInit();
// }
#endif

//----------------------------------------------------------------------------------
void InitImGui()
{
  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize = ImVec2((float)g_width, (float)g_height);        // Display size, in pixels. For clamping windows positions.
  io.DeltaTime = 1.0f/60.0f;                          // Time elapsed since last frame, in seconds (in this sample app we'll override this every frame because our timestep is variable)
  io.PixelCenterOffset = 0.0f;                        // Align OpenGL texels
  io.KeyMap[ImGuiKey_Tab] = sf::Keyboard::Tab;             // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
  io.KeyMap[ImGuiKey_LeftArrow] = sf::Keyboard::Left;
  io.KeyMap[ImGuiKey_RightArrow] = sf::Keyboard::Right;
  io.KeyMap[ImGuiKey_UpArrow] = sf::Keyboard::Up;
  io.KeyMap[ImGuiKey_DownArrow] = sf::Keyboard::Down;
  io.KeyMap[ImGuiKey_Home] = sf::Keyboard::Home;
  io.KeyMap[ImGuiKey_End] = sf::Keyboard::End;
  io.KeyMap[ImGuiKey_Delete] = sf::Keyboard::Delete;
  io.KeyMap[ImGuiKey_Backspace] = sf::Keyboard::BackSpace;
  io.KeyMap[ImGuiKey_Enter] = sf::Keyboard::Return;
  io.KeyMap[ImGuiKey_Escape] = sf::Keyboard::Escape;
  io.KeyMap[ImGuiKey_A] = sf::Keyboard::A;
  io.KeyMap[ImGuiKey_C] = sf::Keyboard::C;
  io.KeyMap[ImGuiKey_V] = sf::Keyboard::V;
  io.KeyMap[ImGuiKey_X] = sf::Keyboard::X;
  io.KeyMap[ImGuiKey_Y] = sf::Keyboard::Y;
  io.KeyMap[ImGuiKey_Z] = sf::Keyboard::Z;

  io.RenderDrawListsFn = ImImpl_RenderDrawLists;
  //io.SetClipboardTextFn = ImImpl_SetClipboardTextFn;
  //io.GetClipboardTextFn = ImImpl_GetClipboardTextFn;

  // Load font texture
  glGenTextures(1, &g_fontTex);

  glBindTexture(GL_TEXTURE_2D, g_fontTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  const void* png_data;
  unsigned int png_size;
  ImGui::GetDefaultFontData(NULL, NULL, &png_data, &png_size);
  int tex_x, tex_y, tex_comp;
  void* tex_data = stbi_load_from_memory((const unsigned char*)png_data, (int)png_size, &tex_x, &tex_y, &tex_comp, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_x, tex_y, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);
  stbi_image_free(tex_data);
}

//----------------------------------------------------------------------------------
void UpdateImGui()
{
  ImGuiIO& io = ImGui::GetIO();

  static sf::Clock ticker;
  io.DeltaTime = max(.0001f, ticker.getElapsedTime().asSeconds());

  // Setup inputs
  // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
  // TODO: handle keypresses etc
  double mouse_x, mouse_y;
  Vector2i mousePos = sf::Mouse::getPosition(*CITYGEN._renderWindow);
  mouse_x = mousePos.x;
  mouse_y = mousePos.y;
  io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);                           // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
  io.MouseDown[0] = sf::Mouse::isButtonPressed(sf::Mouse::Left);
  io.MouseDown[1] = sf::Mouse::isButtonPressed(sf::Mouse::Right);

  // Start the frame
  ImGui::NewFrame();
}


//----------------------------------------------------------------------------------
Terrain::Terrain()
{
  memset(this, 0, sizeof(Terrain));
  scale = 10;
  heightScale = 1;
}

//----------------------------------------------------------------------------------
void Terrain::CreateMesh()
{
  indices.resize((w-1) * (h-1) * 2 * 3);
  verts.resize(w*h);
  vec3* vertPtr = verts.data();
  u32* indPtr = indices.data();

  float z = -(h-1)/2.f * scale;
  u8* ptr = data;

  for (int i = 0; i < h; ++i)
  {
    float x = -(w-1)/2.f * scale;
    for (int j = 0; j < w; ++j)
    {
      float y = (ptr[0] + ptr[1] + ptr[2]) / 3.f * heightScale;

      *vertPtr++ = vec3(x, y, z);

      if (i < h-1 && j < w-1)
      {
        // v1-v2
        // v0-v3
        u32 v0 = (i+0)*w+(j+0);
        u32 v1 = (i+1)*w+(j+0);
        u32 v2 = (i+1)*w+(j+1);
        u32 v3 = (i+0)*w+(j+1);

        indPtr[0] = v0;
        indPtr[1] = v1;
        indPtr[2] = v2;

        indPtr[3] = v0;
        indPtr[4] = v2;
        indPtr[5] = v3;

        indPtr += 6;
      }

      ptr += 4;
      x += scale;
    }
    z += scale;
  }
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
CityGen::CityGen()
  : _renderWindow(nullptr)
  , _eventManager(nullptr)
  , _virtualWindowManager(nullptr)
  //, _lastUpdate(boost::posix_time::not_a_date_time)
  //, _curTime(seconds(0))
  //, _fileWatchAcc(seconds(0))
{
  _stateFlags.Set(StateFlagsF::Paused);
}

//----------------------------------------------------------------------------------
CityGen::~CityGen()
{
  delete _renderWindow;
  delete _eventManager;
  delete _virtualWindowManager;
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

#ifdef _WIN32
  g_width = GetSystemMetrics(SM_CXFULLSCREEN);
  g_height = GetSystemMetrics(SM_CYFULLSCREEN);
#else
  auto displayId = CGMainDisplayID();
  g_width = CGDisplayPixelsWide(displayId);
  g_height = CGDisplayPixelsHigh(displayId);
  _appRoot = "/Users/dooz/projects/boba/editor/";
#endif

  g_width = (u32)(0.9f * g_width);
  g_height = (u32)(0.9 * g_height);

  sf::ContextSettings settings;
  settings.majorVersion = 4;
  settings.minorVersion = 1;
  _renderWindow = new RenderWindow(sf::VideoMode(g_width, g_height), "...", sf::Style::Default, settings);
  _renderWindow->setVerticalSyncEnabled(true);
  _eventManager = new WindowEventManager(_renderWindow);

  _eventManager->RegisterHandler(Event::KeyPressed, bind(&CityGen::OnKeyPressed, this, _1));
  _eventManager->RegisterHandler(Event::KeyReleased, bind(&CityGen::OnKeyReleased, this, _1));
  _eventManager->RegisterHandler(Event::TextEntered, bind(&CityGen::OnTextEntered, this, _1));
  _eventManager->RegisterHandler(Event::LostFocus, bind(&CityGen::OnLostFocus, this, _1));
  _eventManager->RegisterHandler(Event::GainedFocus, bind(&CityGen::OnGainedFocus, this, _1));
  _eventManager->RegisterHandler(Event::MouseButtonReleased, bind(&CityGen::OnMouseButtonReleased, this, _1));

  // add the editor windows
  {
    _virtualWindowManager = new VirtualWindowManager(_renderWindow, _eventManager);

    float w = (float)g_width/4;
    float h = (float)g_height/2;
  }

  _terrain.data = stbi_load("/Users/dooz/projects/citygen/noise.tga", &_terrain.w, &_terrain.h, &_terrain.depth, 0);
  _terrain.CreateMesh();

  //    int x,y,n;
//    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
//    // ... process data if not NULL ...
//    // ... x = width, y = height, n = # 8-bit components per pixel ...
//    // ... replace '0' with '1'..'4' to force that many components per pixel
//    // ... but 'n' will always be the number that it would have been if you said 0
//    stbi_image_free(data)

  InitImGui();

  return true;
}

//----------------------------------------------------------------------------------
bool CityGen::OnLostFocus(const Event& event)
{
  return false;
}

//----------------------------------------------------------------------------------
bool CityGen::OnGainedFocus(const Event& event)
{
  return false;
}

//----------------------------------------------------------------------------------
bool CityGen::OnKeyPressed(const Event& event)
{
  Keyboard::Key key = event.key.code;

  ImGuiIO& io = ImGui::GetIO();
  io.KeysDown[key] = true;

  //   ImGuiIO& io = ImGui::GetIO();
  //   if (action == GLFW_PRESS)
  //     io.KeysDown[key] = true;
  //   if (action == GLFW_RELEASE)
  //     io.KeysDown[key] = false;
  //   io.KeyCtrl = (mods & GLFW_MOD_CONTROL) != 0;
  //   io.KeyShift = (mods & GLFW_MOD_SHIFT) != 0;

  switch (key)
  {
    case Keyboard::Escape: _stateFlags.Set(StateFlagsF::Done); return true;
  }
  return false;
}

//----------------------------------------------------------------------------------
bool CityGen::OnTextEntered(const Event& event)
{
  u32 c = event.text.unicode;
  if (c <= 255)
    ImGui::GetIO().AddInputCharacter((char)c);

  return false;
}

//----------------------------------------------------------------------------------
bool CityGen::OnKeyReleased(const Event& event)
{
  Keyboard::Key key = event.key.code;

  ImGuiIO& io = ImGui::GetIO();
  io.KeysDown[key] = false;

  switch (key)
  {
    case Keyboard::Key::Space: _stateFlags.Toggle(StateFlagsF::Paused); return true;
  }

  return false;
}

//----------------------------------------------------------------------------------
bool CityGen::OnMouseButtonReleased(const Event& event)
{
  return false;
}

//----------------------------------------------------------------------------------
void CityGen::Update()
{
//   ptime now = microsec_clock::local_time();
// 
//   WebbyServerUpdate(_server);
// 
//   if (_lastUpdate.is_not_a_date_time())
//     _lastUpdate = now;
// 
//   time_duration delta = now - _lastUpdate;
//   _fileWatchAcc += delta;
// 
//   if (_fileWatchAcc > seconds(1))
//   {
//     _fileWatchAcc -= seconds(1);
//     _fileWatcher.Tick();
//   }
//   _lastUpdate = now;
// 
//   if (!_stateFlags.IsSet(StateFlagsF::Paused))
//   {
//     _curTime += delta;
//   }

  _eventManager->Poll();
  _virtualWindowManager->Update();
}

//----------------------------------------------------------------------------------
void CityGen::Render()
{
  _renderWindow->clear();

  UpdateImGui();

//  _renderWindow->pushGLStates();

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0f, 1.333f, 1.f, 1000.0f);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0.5f, 300, 200.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

//  glEnable(GL_DEPTH_TEST);
//  glDepthFunc(GL_LEQUAL);
//  glEnable(GL_CULL_FACE);

  glEnable(GL_LINE_SMOOTH);
  glLineWidth(1.2f);

  glColor4ub(0xfd, 0xf6, 0xe3, 255);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, _terrain.verts.data());
  glDrawElements(GL_TRIANGLES, _terrain.indices.size(), GL_UNSIGNED_INT, _terrain.indices.data());
  glDisableClientState(GL_VERTEX_ARRAY);

//  _virtualWindowManager->Draw();
//  _renderWindow->popGLStates();

//  _renderWindow->resetGLStates();
  static bool open = true;
  ImGui::Begin("Properties", &open, ImVec2(200, 100));

  if (ImGui::InputFloat("scale", &_terrain.scale, 1) || ImGui::InputFloat("h-scale", &_terrain.heightScale, 0.1f))
  {
    _terrain.scale = max(1.f, min(100.f, _terrain.scale));
    _terrain.heightScale = max(0.1f, min(5.f, _terrain.heightScale));
    _terrain.CreateMesh();
  }

  ImGui::End();

  ImGui::Render();

  _renderWindow->display();
}

//----------------------------------------------------------------------------------
bool CityGen::Run()
{
  while (_renderWindow->isOpen() && !_stateFlags.IsSet(StateFlagsF::Done))
  {
    Update();
    Render();
  }

  return true;
}

//----------------------------------------------------------------------------------
bool CityGen::Close()
{
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

//---------------------------------------------------------------------------------
//
vec4 Unproject(const vec4& v, const mat4& view, const mat4& proj)
{
  mat4 inv = glm::inverse(proj * view);
  return inv * v;
}

#if 0
  static void RenderFrame()
  {
    glClearColor(0 / 255.0f, 0x2b / 255.0f, 0x36 / 255.0f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // ---- UI

    imguiBeginFrame(mouseX, height - 1 - mouseY, mouseButtons, 0);

    static const int paramSize = 200;
    static int paramScroll = 0;
    imguiBeginScrollArea("Params", 0, height - paramSize, width, paramSize, &paramScroll);

    static float xPos = 0.0f;
    imguiSlider("xPos", &xPos, -1000, 1000.0f, 1);

    static float yPos = 20.0f;
    imguiSlider("yPos", &yPos, -1000, 1000.0f, 1);

    static float zPos = 1000.0f;
    imguiSlider("zPos", &zPos, 1, 1000.0f, 1);

    char buf[256];
    sprintf(buf, "mouse: %d, %d", mouseX, mouseY);
    imguiLabel(buf);

    float nearPlane = 2;
    float farPlane = 10000;

    vec3 pos(xPos, yPos, zPos);
    mat4 view = glm::lookAt(pos, vec3(0, 0, 0), vec3(0, 1, 0));
    mat4 proj = glm::perspective(45.0f, 4.0f / 3.0f, nearPlane, farPlane);

    vec4 r = proj * vec4(0,0,-nearPlane, 1);

    vec2 vecNdc((2.0f * mouseX) / width - 1, 1.0f - (2.0f * mouseY) / height);
    vec4 vecClip(vecNdc.x, vecNdc.y, -1, 1);

    // multiply by the perspective divide
    vecClip *= nearPlane;

    vec4 nearPos = Unproject(vecClip, view, proj);
    vec3 np(nearPos.x, nearPos.y, nearPos.z);
    vec3 d = glm::normalize(np - pos);

    // check if the ray intersects the sphere
    float a = dot(d,d);
    float b = 2 * dot(pos - sphereCenter, d);
    float c = dot(pos - sphereCenter, pos - sphereCenter) - sphereRadius * sphereRadius;

    bool hit = false;
    float t1=0, t2=0;
    float discriminant = sqrtf(b*b - 4*a*c);
    if (discriminant > 0)
    {
      hit = true;
      t1 = (-b - discriminant) / (2*a);
      t2 = (-b + discriminant) / (2*a);
    }

    sprintf(buf, "dir: %f, %f, %f. t: %f, hit: %s", d.x, d.y, d.z, min(t1,t2), hit ? "Y" : "N");
    imguiLabel(buf);

    imguiEndScrollArea();
    imguiEndFrame();

    // render stuff
    glUseProgram(shaderProgram);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(proj));
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(view));

    glPointSize(2);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, heightVbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_POINTS, 0, 256*256);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_POINTS, 0, 10000);

    glLineWidth(1);

    glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    if (vec3* ptr = (vec3*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY))
    {
      ptr[0] = np;
      ptr[1] + np + 10.0f * d;
      glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    glDrawArrays(GL_LINES, 0, 2);

    glUseProgram(0);

    // back to GUI
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1.0f, 1.0f);

    imguiRenderGLDraw();

    glDisable(GL_BLEND);

    SDL_GL_SwapWindow(displayWindow);
  }

  //---------------------------------------------------------------------------------
  //
  bool Init()
  {
    // find the app.root
    string cur = CurrentDirectory();
    string prev = cur;
    while (true)
    {
      if (FileExists("app.root"))
      {
        break;
      }

      chdir("..");

      // break if at the root
      string tmp = CurrentDirectory();
      if (tmp == prev)
      {
        break;
      }
      prev = tmp;
    }

    return true;
  }

  //---------------------------------------------------------------------------------
  //
  GLuint loadShader(const char* filename, GLuint type)
  {
    GLuint shader = glCreateShader(type);
    vector<char> buf;
    if (LoadTextFile(filename, &buf) != Error::OK)
    {
      return -1;
    }

    const char* data = buf.data();
    glShaderSource(shader, 1, &data, NULL);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
      GLint length;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
      vector<char> buf(length);
      glGetShaderInfoLog(shader, length, &length, buf.data());
      printf("%s", buf.data());
    }

    return shader;
  }

  //---------------------------------------------------------------------------------
  //
  Error InitShaders()
  {
    shaderProgram   = glCreateProgram();
    vertexShader    = loadShader("simple.vert.glsl", GL_VERTEX_SHADER);
    fragmentShader  = loadShader("simple.frag.glsl", GL_FRAGMENT_SHADER);

    if (vertexShader == -1 || fragmentShader == -1)
    {
      return Error::INVALID_SHADER;
    }

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    glLinkProgram(shaderProgram);
    return Error::OK;
  }

  //---------------------------------------------------------------------------------
  //
  void GenerateSphere()
  {
    vector<vec3> verts;

    while (verts.size() < 10000)
    {
      float x = -1 + 2 * (rand() / (float)RAND_MAX);
      float y = -1 + 2 * (rand() / (float)RAND_MAX);
      float z = -1 + 2 * (rand() / (float)RAND_MAX);

      vec3 p(x,y,z);
      p *= sphereRadius;

      if (glm::length(p) < sphereRadius)
      {
        verts.push_back(p + sphereCenter);
      }
    }

    glGenBuffers(1, &sphereVbo);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(vec3), verts.data(), GL_STATIC_DRAW);
  }

  //---------------------------------------------------------------------------------
  //
  Error CreateHeightField()
  {
    vector<u8> buf;
    Error err = LoadFile("noise.tga", &buf);
    if (err != Error::OK)
      return err;

    vector<vec3> verts;
    int width = 256;
    int height = 256;

    u8* data = &buf[18];

    float gridSpacing = 10;
    float zPos = -(width/2 - 0.5f) * gridSpacing;

    // store height info
    for (int y = 0; y < height; ++y)
    {
      float xPos = -(width/2 - 0.5f) * gridSpacing;
      for (int x = 0; x < width; ++x)
      {
        float h = data[(x+y*width)*4];
        verts.push_back(vec3(xPos, h, zPos));
        xPos += gridSpacing;
      }
      zPos += gridSpacing;
    }

    glGenBuffers(1, &heightVbo);
    glBindBuffer(GL_ARRAY_BUFFER, heightVbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(vec3), verts.data(), GL_STATIC_DRAW);

    return Error::OK;
  }
}


int main(int argc, char** argv)
{
  using namespace citygen;
  Init();
  width = 1024;
  height = 768;
  SDL_SetMainReady();
  SDL_Init(SDL_INIT_VIDEO);
  displayWindow = SDL_CreateWindow(
    "SDL2/OpenGL Demo", 100, 100, width, height, 
    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

  SDL_GLContext glcontext = SDL_GL_CreateContext(displayWindow);
  glViewport(0, 0, width, height);

  GLuint res = glewInit();
  if (res != GLEW_OK)
  {
    return 1;
  }

  imguiRenderGLInit("calibri.ttf");

  Error err = InitShaders();
  if (err != Error::OK)
  {
    return (int)err;
  }

  if (CreateHeightField() != Error::OK)
    return 1;

  GenerateSphere();

  glGenBuffers(1, &lineVbo);
  glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
  glBufferData(GL_ARRAY_BUFFER, 2 * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

  bool quit = false;
  while (!quit)
  {
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
      switch (event.type)
      {
      case SDL_QUIT:
        quit = true;
        break;

      case SDL_MOUSEMOTION:
        mouseX = event.motion.x;
        mouseY = event.motion.y;
        break;

      case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT)
        {
          mouseButtons |= 1;
        }
        else if (event.button.button == SDL_BUTTON_RIGHT)
        {
          mouseButtons |= 2;
        }
        break;

      case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT)
        {
          mouseButtons &= ~1;
        }
        else if (event.button.button == SDL_BUTTON_RIGHT)
        {
          mouseButtons &= ~2;
        }
        break;
      }
    }
    RenderFrame();
  }

  SDL_GL_DeleteContext(glcontext);
  SDL_Quit();

  return 0;
}
#endif